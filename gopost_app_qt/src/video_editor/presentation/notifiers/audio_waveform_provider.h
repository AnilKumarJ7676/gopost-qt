#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QImage>
#include <QMutex>
#include <QCache>
#include <QString>
#include <QRunnable>
#include <QThreadPool>
#include <QMap>
#include <QList>
#include <QSet>
#include <QTimer>
#include <QObject>
#include <QPointer>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// WaveformCache — thread-safe in-memory cache for rendered waveform images
// ---------------------------------------------------------------------------
class WaveformCache {
public:
    WaveformCache(int maxEntries = 300);

    bool contains(const QString& key) const;
    QImage get(const QString& key) const;
    void insert(const QString& key, const QImage& img);
    void clear();

private:
    mutable QMutex mutex_;
    mutable QCache<QString, QImage> cache_;
};

// ---------------------------------------------------------------------------
// WaveformResponse — async response with thread-safe alive flag
// ---------------------------------------------------------------------------
class WaveformResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    WaveformResponse();
    ~WaveformResponse() override;

    void deliver(const QImage& img);
    QQuickTextureFactory* textureFactory() const override;

    std::shared_ptr<std::atomic<bool>> aliveFlag() const { return alive_; }

private:
    QImage result_;
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
};

// ---------------------------------------------------------------------------
// WaveformCoordinator — batches requests and dispatches workers
// ---------------------------------------------------------------------------
class WaveformCoordinator : public QObject {
    Q_OBJECT
public:
    struct PendingRequest {
        double startSec;
        double endSec;
        int pixelWidth;
        int pixelHeight;
        int channels;    // 1=mono, 2=stereo
        WaveformResponse* response;
        std::shared_ptr<std::atomic<bool>> alive;
    };

    explicit WaveformCoordinator(WaveformCache* cache, QThreadPool* pool,
                                  QObject* parent = nullptr);

    void enqueue(const QString& filePath, const PendingRequest& req);
    Q_INVOKABLE void onFileComplete(const QString& filePath);

public slots:
    void dispatchBatches();

private:
    WaveformCache* cache_;
    QThreadPool* pool_;
    QTimer collectTimer_;

    QMutex mutex_;
    QMap<QString, QList<PendingRequest>> pending_;
    QSet<QString> activeFiles_;
};

// ---------------------------------------------------------------------------
// WaveformWorker — extracts audio peaks via ffmpeg and renders waveform image
// ---------------------------------------------------------------------------
class WaveformWorker : public QRunnable {
public:
    struct Request {
        double startSec;
        double endSec;
        int pixelWidth;
        int pixelHeight;
        int channels;
        WaveformResponse* response;
        std::shared_ptr<std::atomic<bool>> alive;
    };

    WaveformWorker(const QString& filePath, QList<Request> requests,
                   WaveformCache* cache, WaveformCoordinator* coordinator);
    void run() override;

private:
    QString filePath_;
    QList<Request> requests_;
    WaveformCache* cache_;
    WaveformCoordinator* coordinator_;

    // Extract audio peaks from file for a time range
    std::vector<float> extractPeaks(const QString& filePath, double startSec,
                                     double endSec, int numSamples, int channels);

    // Render waveform peaks into a QImage
    QImage renderWaveform(const std::vector<float>& peaks, int width, int height,
                          int channels);
};

// ---------------------------------------------------------------------------
// AudioWaveformProvider — QML async image provider
//
// URL: image://audiowaveform/<base64_path>@<startSec>@<endSec>@<width>@<height>@<channels>
// ---------------------------------------------------------------------------
class AudioWaveformProvider : public QQuickAsyncImageProvider {
public:
    AudioWaveformProvider();
    ~AudioWaveformProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                               const QSize& requestedSize) override;

private:
    WaveformCache cache_;
    QThreadPool pool_;
    WaveformCoordinator* coordinator_;
};

} // namespace gopost::video_editor
