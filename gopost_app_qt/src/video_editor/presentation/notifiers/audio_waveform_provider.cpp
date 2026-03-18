#include "video_editor/presentation/notifiers/audio_waveform_provider.h"

#include <QColor>
#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>
#include <QPainter>
#include <QProcess>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gopost::video_editor {

// ── WaveformCache ────────────────────────────────────────────────────────

WaveformCache::WaveformCache(int maxEntries) : cache_(maxEntries) {}

bool WaveformCache::contains(const QString& key) const {
    QMutexLocker lock(&mutex_);
    return cache_.contains(key);
}

QImage WaveformCache::get(const QString& key) const {
    QMutexLocker lock(&mutex_);
    auto* img = cache_.object(key);
    return img ? *img : QImage();
}

void WaveformCache::insert(const QString& key, const QImage& img) {
    QMutexLocker lock(&mutex_);
    cache_.insert(key, new QImage(img));
}

void WaveformCache::clear() {
    QMutexLocker lock(&mutex_);
    cache_.clear();
}

// ── WaveformResponse ─────────────────────────────────────────────────────

WaveformResponse::WaveformResponse() : QQuickImageResponse() {}

WaveformResponse::~WaveformResponse() {
    alive_->store(false, std::memory_order_release);
}

void WaveformResponse::deliver(const QImage& img) {
    result_ = img;
    emit finished();
}

QQuickTextureFactory* WaveformResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(result_);
}

// ── WaveformCoordinator ──────────────────────────────────────────────────

WaveformCoordinator::WaveformCoordinator(WaveformCache* cache, QThreadPool* pool,
                                           QObject* parent)
    : QObject(parent), cache_(cache), pool_(pool) {
    collectTimer_.setSingleShot(true);
    collectTimer_.setInterval(200);  // 200ms collection window — debounces rapid zoom
    connect(&collectTimer_, &QTimer::timeout, this, &WaveformCoordinator::dispatchBatches);
}

void WaveformCoordinator::enqueue(const QString& filePath, const PendingRequest& req) {
    QMutexLocker lock(&mutex_);
    pending_[filePath].append(req);
    if (!collectTimer_.isActive())
        QMetaObject::invokeMethod(&collectTimer_, "start", Qt::QueuedConnection);
}

void WaveformCoordinator::dispatchBatches() {
    QMutexLocker lock(&mutex_);
    if (pending_.isEmpty()) return;

    int totalRequests = 0;
    for (auto it = pending_.begin(); it != pending_.end(); ) {
        const QString& filePath = it.key();

        if (activeFiles_.contains(filePath)) { ++it; continue; }

        QList<WaveformWorker::Request> workerRequests;
        for (const auto& req : it.value()) {
            if (!req.alive || !req.alive->load(std::memory_order_acquire)) continue;
            workerRequests.append({req.startSec, req.endSec, req.pixelWidth,
                                   req.pixelHeight, req.channels,
                                   req.response, req.alive});
        }

        if (!workerRequests.isEmpty()) {
            totalRequests += workerRequests.size();
            activeFiles_.insert(filePath);
            auto* worker = new WaveformWorker(filePath, std::move(workerRequests),
                                               cache_, this);
            worker->setAutoDelete(true);
            pool_->start(worker);
        }
        it = pending_.erase(it);
    }

    if (totalRequests > 0)
        qDebug() << "[Waveform] dispatched" << totalRequests << "requests";
}

void WaveformCoordinator::onFileComplete(const QString& filePath) {
    {
        QMutexLocker lock(&mutex_);
        activeFiles_.remove(filePath);
    }
    dispatchBatches();
}

// ── WaveformWorker ───────────────────────────────────────────────────────

WaveformWorker::WaveformWorker(const QString& filePath, QList<Request> requests,
                                WaveformCache* cache, WaveformCoordinator* coordinator)
    : filePath_(filePath), requests_(std::move(requests)),
      cache_(cache), coordinator_(coordinator) {}

void WaveformWorker::run() {
    if (requests_.isEmpty()) {
        QMetaObject::invokeMethod(coordinator_, "onFileComplete",
                                  Qt::QueuedConnection, Q_ARG(QString, filePath_));
        return;
    }

    for (auto& req : requests_) {
        if (!req.alive || !req.alive->load(std::memory_order_acquire)) continue;

        double visDuration = req.endSec - req.startSec;
        if (visDuration <= 0) continue;

        QString cacheKey = filePath_ + "@" + QString::number(req.startSec, 'f', 2) +
                           "@" + QString::number(req.endSec, 'f', 2) +
                           "@" + QString::number(req.pixelWidth) +
                           "@" + QString::number(req.channels);

        QImage img = cache_->get(cacheKey);
        if (!img.isNull()) {
            auto alive = req.alive;
            auto* resp = req.response;
            QMetaObject::invokeMethod(coordinator_, [alive, resp, img]() {
                if (alive->load(std::memory_order_acquire)) resp->deliver(img);
            }, Qt::QueuedConnection);
            continue;
        }

        auto peaks = extractPeaks(filePath_, req.startSec, req.endSec,
                                   req.pixelWidth, req.channels);
        img = renderWaveform(peaks, req.pixelWidth, req.pixelHeight, req.channels);
        cache_->insert(cacheKey, img);

        auto alive = req.alive;
        auto* resp = req.response;
        QMetaObject::invokeMethod(coordinator_, [alive, resp, img]() {
            if (alive->load(std::memory_order_acquire)) resp->deliver(img);
        }, Qt::QueuedConnection);
    }

    QMetaObject::invokeMethod(coordinator_, "onFileComplete",
                              Qt::QueuedConnection, Q_ARG(QString, filePath_));
}

std::vector<float> WaveformWorker::extractPeaks(const QString& filePath,
                                                  double startSec, double endSec,
                                                  int numSamples, int channels) {
    std::vector<float> peaks(numSamples * channels, 0.0f);
    double duration = endSec - startSec;
    if (duration <= 0 || numSamples <= 0) return peaks;

    // Target sample rate: scale with visible duration for performance.
    // Short ranges (few seconds): high quality. Long ranges (minutes): low sample rate.
    int oversample = duration > 60.0 ? 8 : (duration > 10.0 ? 16 : 64);
    int rawSamplesNeeded = numSamples * oversample;
    int sampleRate = std::clamp(static_cast<int>(rawSamplesNeeded / duration),
                                 4000, 22050);

    QProcess ffmpeg;
    ffmpeg.setProcessChannelMode(QProcess::SeparateChannels);

    QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-ss"), QString::number(startSec, 'f', 3),
        QStringLiteral("-t"), QString::number(std::min(duration, 120.0), 'f', 3),  // cap at 2 min
        QStringLiteral("-i"), filePath,
        QStringLiteral("-vn"),  // no video
        QStringLiteral("-ac"), QString::number(channels),
        QStringLiteral("-ar"), QString::number(sampleRate),
        QStringLiteral("-f"), QStringLiteral("s16le"),
        QStringLiteral("-acodec"), QStringLiteral("pcm_s16le"),
        QStringLiteral("pipe:1")
    };

    ffmpeg.start(QStringLiteral("ffmpeg"), args);
    if (!ffmpeg.waitForStarted(3000)) {
        return peaks;
    }
    if (!ffmpeg.waitForFinished(8000)) {
        ffmpeg.kill();
        ffmpeg.waitForFinished(500);
        return peaks;
    }

    QByteArray rawData = ffmpeg.readAllStandardOutput();
    if (rawData.isEmpty()) return peaks;

    // Parse s16le PCM samples
    int totalRawSamples = rawData.size() / (2 * channels);
    if (totalRawSamples == 0) return peaks;

    const int16_t* samples = reinterpret_cast<const int16_t*>(rawData.constData());
    int samplesPerBin = std::max(1, totalRawSamples / numSamples);

    for (int bin = 0; bin < numSamples; ++bin) {
        int binStart = bin * totalRawSamples / numSamples;
        int binEnd = std::min((bin + 1) * totalRawSamples / numSamples, totalRawSamples);

        for (int ch = 0; ch < channels; ++ch) {
            float maxAmp = 0.0f;
            for (int s = binStart; s < binEnd; ++s) {
                float amp = std::abs(static_cast<float>(samples[s * channels + ch]) / 32768.0f);
                maxAmp = std::max(maxAmp, amp);
            }
            peaks[bin * channels + ch] = maxAmp;
        }
    }

    return peaks;
}

QImage WaveformWorker::renderWaveform(const std::vector<float>& peaks,
                                       int width, int height, int channels) {
    QImage img(width, height, QImage::Format_RGBA8888);
    img.fill(Qt::transparent);

    if (peaks.empty() || width <= 0 || height <= 0) return img;

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Waveform colors — cyan for mono/left, green for right
    QColor colorL(38, 198, 218, 180);   // #26C6DA
    QColor colorR(102, 187, 106, 180);  // #66BB6A

    if (channels == 1) {
        // Mono: mirrored waveform centered vertically
        float centerY = height / 2.0f;
        painter.setPen(Qt::NoPen);

        for (int x = 0; x < width && x < static_cast<int>(peaks.size()); ++x) {
            float amp = peaks[x];
            float barH = amp * centerY;
            if (barH < 0.5f) barH = 0.5f;

            // Draw mirrored bar
            painter.fillRect(QRectF(x, centerY - barH, 1, barH * 2), colorL);
        }

        // Center line
        painter.setPen(QPen(QColor(38, 198, 218, 60), 0.5));
        painter.drawLine(0, static_cast<int>(centerY), width, static_cast<int>(centerY));
    } else {
        // Stereo: top half = left, bottom half = right
        float halfH = height / 2.0f;
        painter.setPen(Qt::NoPen);

        for (int x = 0; x < width; ++x) {
            int idx = x * channels;
            if (idx + 1 >= static_cast<int>(peaks.size())) break;

            float ampL = peaks[idx];
            float ampR = peaks[idx + 1];

            // Left channel: draws downward from top center line
            float barHL = std::max(0.5f, ampL * halfH);
            painter.fillRect(QRectF(x, halfH - barHL, 1, barHL), colorL);

            // Right channel: draws downward from bottom center line
            float barHR = std::max(0.5f, ampR * halfH);
            painter.fillRect(QRectF(x, halfH, 1, barHR), colorR);
        }

        // Center divider
        painter.setPen(QPen(QColor(255, 255, 255, 30), 0.5));
        painter.drawLine(0, static_cast<int>(halfH), width, static_cast<int>(halfH));
    }

    painter.end();
    return img;
}

// ── AudioWaveformProvider ────────────────────────────────────────────────

AudioWaveformProvider::AudioWaveformProvider()
    : QQuickAsyncImageProvider() {
    pool_.setMaxThreadCount(2);
    coordinator_ = new WaveformCoordinator(&cache_, &pool_);
    qDebug() << "[Waveform] provider created: pool=" << pool_.maxThreadCount();
}

AudioWaveformProvider::~AudioWaveformProvider() {
    pool_.waitForDone(3000);
    delete coordinator_;
}

QQuickImageResponse* AudioWaveformProvider::requestImageResponse(
    const QString& id, const QSize& requestedSize) {
    Q_UNUSED(requestedSize)

    // Parse URL: <base64_path>@<startSec>@<endSec>@<width>@<height>@<channels>
    auto parts = id.split('@');
    if (parts.size() < 6) {
        auto* response = new WaveformResponse();
        response->deliver(QImage());
        return response;
    }

    QString pathBase64 = parts[0];
    double startSec = parts[1].toDouble();
    double endSec = parts[2].toDouble();
    int width = parts[3].toInt();
    int height = parts[4].toInt();
    int channels = parts[5].toInt();

    if (width <= 0) width = 100;
    if (width > 2000) width = 2000;  // cap to avoid massive images
    if (height <= 0) height = 24;
    if (height > 100) height = 100;
    if (channels < 1) channels = 1;
    if (channels > 2) channels = 2;

    QString filePath = QString::fromUtf8(QByteArray::fromBase64(pathBase64.toUtf8()));

    // Check cache
    QString cacheKey = filePath + "@" + QString::number(startSec, 'f', 2) +
                       "@" + QString::number(endSec, 'f', 2) +
                       "@" + QString::number(width) +
                       "@" + QString::number(channels);
    QImage cached = cache_.get(cacheKey);
    if (!cached.isNull()) {
        auto* response = new WaveformResponse();
        response->deliver(cached);
        return response;
    }

    // Queue for async extraction
    auto* response = new WaveformResponse();
    WaveformCoordinator::PendingRequest req{
        startSec, endSec, width, height, channels,
        response, response->aliveFlag()
    };
    coordinator_->enqueue(filePath, req);
    return response;
}

} // namespace gopost::video_editor
