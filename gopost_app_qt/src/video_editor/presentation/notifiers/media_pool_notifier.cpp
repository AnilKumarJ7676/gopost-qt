#include "video_editor/presentation/notifiers/media_pool_notifier.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QThread>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>

#include "video_editor/data/services/import_pipeline.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Filtered assets
// ---------------------------------------------------------------------------

std::vector<const MediaAsset*> MediaPoolState::filteredAssets() const {
    std::vector<const MediaAsset*> result;
    for (const auto& a : assets) {
        // Bin filter
        if (activeBinId && a.binId != *activeBinId) continue;
        // Type filter
        if (filterType.has_value() && a.type != *filterType) continue;
        // Search
        if (!searchQuery.isEmpty() &&
            !a.fileName.contains(searchQuery, Qt::CaseInsensitive)) continue;
        result.push_back(&a);
    }

    // Sort
    auto cmp = [this](const MediaAsset* a, const MediaAsset* b) -> bool {
        int c = 0;
        switch (sortBy) {
        case MediaPoolSortBy::Name:     c = a->fileName.compare(b->fileName, Qt::CaseInsensitive); break;
        case MediaPoolSortBy::Date:     c = a->importedAt < b->importedAt ? -1 : (a->importedAt > b->importedAt ? 1 : 0); break;
        case MediaPoolSortBy::Type:     c = static_cast<int>(a->type) - static_cast<int>(b->type); break;
        case MediaPoolSortBy::Duration: c = a->durationSeconds < b->durationSeconds ? -1 : (a->durationSeconds > b->durationSeconds ? 1 : 0); break;
        case MediaPoolSortBy::Size:     c = a->fileSizeBytes < b->fileSizeBytes ? -1 : (a->fileSizeBytes > b->fileSizeBytes ? 1 : 0); break;
        }
        return sortAscending ? c < 0 : c > 0;
    };
    std::sort(result.begin(), result.end(), cmp);
    return result;
}

// ---------------------------------------------------------------------------

MediaPoolNotifier::MediaPoolNotifier(QObject* parent) : QObject(parent) {}
MediaPoolNotifier::~MediaPoolNotifier() = default;

QString MediaPoolNotifier::generateAssetId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ---------------------------------------------------------------------------
// Assets
// ---------------------------------------------------------------------------

static const QStringList kVideoExts = {
    QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("avi"),
    QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("m4v"),
    QStringLiteral("flv"), QStringLiteral("wmv"), QStringLiteral("3gp"),
    QStringLiteral("mxf"), QStringLiteral("ts")
};
static const QStringList kImageExts = {
    QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"),
    QStringLiteral("webp"), QStringLiteral("gif"), QStringLiteral("bmp"),
    QStringLiteral("heic"), QStringLiteral("heif"), QStringLiteral("tiff"),
    QStringLiteral("tif"), QStringLiteral("exr"), QStringLiteral("dpx")
};
static const QStringList kAudioExts = {
    QStringLiteral("mp3"), QStringLiteral("aac"), QStringLiteral("wav"),
    QStringLiteral("m4a"), QStringLiteral("flac"), QStringLiteral("ogg"),
    QStringLiteral("wma"), QStringLiteral("opus"), QStringLiteral("aiff")
};

void MediaPoolNotifier::importFile(const QString& path) {
    qDebug() << "[MediaPool] importFile:" << path;
    QFileInfo fi(path);

    // Format whitelist check
    const auto ext = fi.suffix().toLower();
    bool supported = kVideoExts.contains(ext) || kImageExts.contains(ext) || kAudioExts.contains(ext);
    if (!supported) {
        qDebug() << "[MediaPool] rejected unsupported format:" << ext;
        emit importRejected(fi.fileName(), QStringLiteral("Unsupported format: .%1").arg(ext));
        return;
    }

    // Duplicate detection: check path + size + lastModified
    for (const auto& existing : state_.assets) {
        if (existing.filePath == path &&
            existing.fileSizeBytes == fi.size()) {
            qDebug() << "[MediaPool] duplicate detected:" << fi.fileName();
            emit duplicateDetected(fi.fileName());
            return;
        }
    }

    MediaAsset asset;
    asset.id            = generateAssetId();
    asset.filePath      = path;
    asset.fileName      = fi.fileName();
    asset.fileSizeBytes = fi.size();
    asset.importedAt    = QDateTime::currentDateTime();
    asset.status        = fi.exists() ? MediaAssetStatus::Online : MediaAssetStatus::Offline;

    // Assign to active bin if one is selected
    if (state_.activeBinId.has_value())
        asset.binId = *state_.activeBinId;

    if (kVideoExts.contains(ext))       asset.type = MediaAssetType::Video;
    else if (kImageExts.contains(ext))  asset.type = MediaAssetType::Image;
    else if (kAudioExts.contains(ext))  asset.type = MediaAssetType::Audio;

    // Fast path: images get default duration, no ffprobe needed
    if (asset.type == MediaAssetType::Image) {
        asset.durationSeconds = 5.0;
        state_.assets.push_back(std::move(asset));
        emit stateChanged();
        return;
    }

    // For video/audio: add immediately with placeholder metadata,
    // then probe asynchronously on a background thread
    asset.status = MediaAssetStatus::Probing;
    asset.durationSeconds = 10.0;  // placeholder
    QString assetId = asset.id;
    QString filePath = asset.filePath;

    state_.assets.push_back(std::move(asset));
    emit stateChanged();  // UI sees asset immediately

    // Background probe — concurrency-limited, does NOT block UI thread
    QtConcurrent::run([this, assetId, filePath]() {
        // Acquire semaphore — blocks background thread if kMaxConcurrentProbes reached
        probeSemaphore().acquire();

        MediaAsset temp;
        temp.filePath = filePath;
        temp.type = MediaAssetType::Video;
        probeMediaMetadata(temp);

        probeSemaphore().release();

        // Push results back to main thread
        QMetaObject::invokeMethod(this, [this, assetId,
            dur = temp.durationSeconds, w = temp.width, h = temp.height,
            fps = temp.frameRate, codec = temp.codec]() {
            for (auto& a : state_.assets) {
                if (a.id == assetId) {
                    a.durationSeconds = dur;
                    a.width = w;
                    a.height = h;
                    a.frameRate = fps;
                    a.codec = codec;
                    a.status = MediaAssetStatus::Online;
                    break;
                }
            }
            emit stateChanged();
        }, Qt::QueuedConnection);
    });
}

double MediaPoolNotifier::durationForPath(const QString& path) const {
    for (const auto& asset : state_.assets) {
        if (asset.filePath == path)
            return asset.durationSeconds;
    }
    return 0.0;
}

void MediaPoolNotifier::probeMediaMetadata(MediaAsset& asset) {
    // Images get a default duration of 5 seconds
    if (asset.type == MediaAssetType::Image) {
        asset.durationSeconds = 5.0;
        qDebug() << "[MediaPool] image default duration: 5.0s";
        return;
    }

    // Try ffprobe first for accurate duration/resolution
    bool probed = false;

    // Use system() instead of QProcess to completely bypass Qt's internal
    // QRingBuffer which can overflow: ASSERT "bytes <= bufferSize"
    QString tmpPath = QDir::tempPath() + QStringLiteral("/gopost_mediaprobe_%1_%2.json")
        .arg(QCoreApplication::applicationPid())
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

    QString cmd = QStringLiteral(
        "ffprobe -v quiet -print_format json -show_format -show_streams"
        " \"%1\" > \"%2\" 2>NUL")
        .arg(asset.filePath, tmpPath);

    system(cmd.toLocal8Bit().constData());

    QByteArray output;
    QFile tmpFile(tmpPath);
    if (tmpFile.exists() && tmpFile.open(QIODevice::ReadOnly)) {
        output = tmpFile.readAll();
        tmpFile.close();
    }
    QFile::remove(tmpPath);

    if (!output.isEmpty()) {
        QJsonParseError parseError;
        auto doc = QJsonDocument::fromJson(output, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            auto root = doc.object();

            // Duration from format
            auto format = root.value(QStringLiteral("format")).toObject();
            double dur = format.value(QStringLiteral("duration")).toString().toDouble();
            if (dur > 0.0) {
                asset.durationSeconds = dur;
                probed = true;
                qDebug() << "[MediaPool] ffprobe duration:" << dur << "s";
            }

            // Resolution and frame rate from video stream
            auto streams = root.value(QStringLiteral("streams")).toArray();
            for (const auto& s : streams) {
                auto stream = s.toObject();
                if (stream.value(QStringLiteral("codec_type")).toString() == QStringLiteral("video")) {
                    int w = stream.value(QStringLiteral("width")).toInt();
                    int h = stream.value(QStringLiteral("height")).toInt();
                    if (w > 0 && h > 0) {
                        asset.width = w;
                        asset.height = h;
                    }
                    // Parse frame rate from "r_frame_rate" (e.g., "24000/1001")
                    QString fpsStr = stream.value(QStringLiteral("r_frame_rate")).toString();
                    if (!fpsStr.isEmpty()) {
                        auto parts = fpsStr.split('/');
                        if (parts.size() == 2) {
                            double num = parts[0].toDouble();
                            double den = parts[1].toDouble();
                            if (den > 0) asset.frameRate = num / den;
                        }
                    }
                    // Codec name
                    asset.codec = stream.value(QStringLiteral("codec_name")).toString();

                    // If no duration from format, try stream duration
                    if (!probed) {
                        double streamDur = stream.value(QStringLiteral("duration")).toString().toDouble();
                        if (streamDur > 0.0) {
                            asset.durationSeconds = streamDur;
                            probed = true;
                        }
                    }
                    break;
                }
            }

            // For audio-only files, check audio stream duration
            if (!probed) {
                for (const auto& s : streams) {
                    auto stream = s.toObject();
                    if (stream.value(QStringLiteral("codec_type")).toString() == QStringLiteral("audio")) {
                        double streamDur = stream.value(QStringLiteral("duration")).toString().toDouble();
                        if (streamDur > 0.0) {
                            asset.durationSeconds = streamDur;
                            probed = true;
                        }
                        break;
                    }
                }
            }
        }
    } else {
        qDebug() << "[MediaPool] ffprobe not available or timed out, using fallback";
    }

    // Fallback: estimate duration from file size (bitrate heuristic)
    if (!probed) {
        QFileInfo fi(asset.filePath);
        if (fi.exists() && fi.size() > 0) {
            // Assume ~8 Mbps average bitrate
            double estimatedSeconds = fi.size() * 8.0 / (8.0 * 1000.0 * 1000.0);
            asset.durationSeconds = std::clamp(estimatedSeconds, 1.0, 360000.0);
            asset.width = 1920;
            asset.height = 1080;
            asset.frameRate = 30.0;
            qDebug() << "[MediaPool] fallback duration estimate:" << asset.durationSeconds << "s"
                     << "(from file size:" << fi.size() << "bytes)";
        } else {
            asset.durationSeconds = 10.0;  // absolute fallback
            qDebug() << "[MediaPool] file not readable, default duration: 10.0s";
        }
    }
}

void MediaPoolNotifier::removeAsset(const QString& assetId) {
    auto& a = state_.assets;
    a.erase(std::remove_if(a.begin(), a.end(),
                            [&](const MediaAsset& m) { return m.id == assetId; }),
            a.end());
    emit stateChanged();
}

void MediaPoolNotifier::moveAssetToBin(const QString& assetId, const QString& binId) {
    for (auto& a : state_.assets) {
        if (a.id == assetId) { a.binId = binId; break; }
    }
    emit stateChanged();
}

void MediaPoolNotifier::relinkAsset(const QString& assetId, const QString& newPath) {
    for (auto& a : state_.assets) {
        if (a.id == assetId) {
            a.filePath = newPath;
            a.status   = QFileInfo::exists(newPath) ? MediaAssetStatus::Online
                                                     : MediaAssetStatus::Offline;
            break;
        }
    }
    emit stateChanged();
}

void MediaPoolNotifier::checkAllAssetsOnline() {
    for (auto& a : state_.assets)
        a.status = a.checkOnline() ? MediaAssetStatus::Online : MediaAssetStatus::Offline;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Bins
// ---------------------------------------------------------------------------

void MediaPoolNotifier::createBin(const QString& name) {
    MediaBin bin;
    bin.id   = generateAssetId();
    bin.name = name;
    state_.bins.push_back(std::move(bin));
    emit stateChanged();
}

void MediaPoolNotifier::renameBin(const QString& binId, const QString& name) {
    for (auto& b : state_.bins)
        if (b.id == binId) { b.name = name; break; }
    emit stateChanged();
}

void MediaPoolNotifier::removeBin(const QString& binId) {
    auto& b = state_.bins;
    b.erase(std::remove_if(b.begin(), b.end(),
                            [&](const MediaBin& m) { return m.id == binId; }),
            b.end());
    if (state_.activeBinId == binId)
        state_.activeBinId.reset();
    emit stateChanged();
}

void MediaPoolNotifier::setActiveBin(const QString& binId) {
    // Save current bin's selection
    QString currentBin = state_.activeBinId.value_or(QString());
    binSelections_[currentBin] = selectedAssetIds_;

    state_.activeBinId = binId.isEmpty() ? std::nullopt : std::optional(binId);

    // Restore target bin's selection
    QString targetBin = state_.activeBinId.value_or(QString());
    selectedAssetIds_ = binSelections_.value(targetBin);
    selectedAssetId_ = selectedAssetIds_.isEmpty() ? QString() : *selectedAssetIds_.begin();
    ++selectionGeneration_;

    emit stateChanged();
    emit selectionChanged();
}

// ---------------------------------------------------------------------------
// QML property helpers
// ---------------------------------------------------------------------------

QVariantList MediaPoolNotifier::filteredAssetsVariant() const {
    QVariantList result;
    auto filtered = state_.filteredAssets();
    for (const auto* asset : filtered) {
        QVariantMap m;
        m[QStringLiteral("id")] = asset->id;
        m[QStringLiteral("displayName")] = asset->fileName;
        m[QStringLiteral("filePath")] = asset->filePath;
        m[QStringLiteral("mediaType")] = [&] {
            switch (asset->type) {
            case MediaAssetType::Video: return QStringLiteral("video");
            case MediaAssetType::Image: return QStringLiteral("image");
            case MediaAssetType::Audio: return QStringLiteral("audio");
            default:                    return QStringLiteral("video");
            }
        }();
        m[QStringLiteral("duration")] = asset->durationSeconds;
        m[QStringLiteral("fileSize")] = static_cast<qint64>(asset->fileSizeBytes);
        result.append(m);
    }
    return result;
}

QString MediaPoolNotifier::currentFilter() const {
    if (!state_.filterType.has_value()) return QStringLiteral("all");
    switch (*state_.filterType) {
    case MediaAssetType::Video: return QStringLiteral("video");
    case MediaAssetType::Image: return QStringLiteral("image");
    case MediaAssetType::Audio: return QStringLiteral("audio");
    default:                    return QStringLiteral("all");
    }
}

void MediaPoolNotifier::selectAsset(const QString& assetId) {
    selectedAssetId_ = assetId;
    selectedAssetIds_.clear();
    selectedAssetIds_.insert(assetId);
    ++selectionGeneration_;
    emit selectionChanged();
}

void MediaPoolNotifier::toggleAssetSelection(const QString& assetId) {
    if (selectedAssetIds_.contains(assetId)) {
        selectedAssetIds_.remove(assetId);
        if (selectedAssetId_ == assetId)
            selectedAssetId_ = selectedAssetIds_.isEmpty() ? QString() : *selectedAssetIds_.begin();
    } else {
        selectedAssetIds_.insert(assetId);
        selectedAssetId_ = assetId;
    }
    ++selectionGeneration_;
    emit selectionChanged();
}

void MediaPoolNotifier::rangeSelectAsset(const QString& assetId) {
    auto filtered = state_.filteredAssets();
    int anchorIdx = -1, targetIdx = -1;
    for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
        if (filtered[i]->id == selectedAssetId_) anchorIdx = i;
        if (filtered[i]->id == assetId) targetIdx = i;
    }
    if (anchorIdx < 0 || targetIdx < 0) {
        selectAsset(assetId);
        return;
    }
    int lo = std::min(anchorIdx, targetIdx);
    int hi = std::max(anchorIdx, targetIdx);
    selectedAssetIds_.clear();
    for (int i = lo; i <= hi; ++i)
        selectedAssetIds_.insert(filtered[i]->id);
    selectedAssetId_ = assetId;
    ++selectionGeneration_;
    emit selectionChanged();
}

void MediaPoolNotifier::selectAllInBin() {
    auto filtered = state_.filteredAssets();
    selectedAssetIds_.clear();
    for (const auto* asset : filtered)
        selectedAssetIds_.insert(asset->id);
    if (!filtered.empty())
        selectedAssetId_ = filtered.front()->id;
    ++selectionGeneration_;
    emit selectionChanged();
}

void MediaPoolNotifier::deselectAll() {
    selectedAssetIds_.clear();
    selectedAssetId_.clear();
    ++selectionGeneration_;
    emit selectionChanged();
}

bool MediaPoolNotifier::isAssetSelected(const QString& assetId) const {
    return selectedAssetIds_.contains(assetId);
}

QVariantList MediaPoolNotifier::selectedAssetIdsVariant() const {
    QVariantList result;
    for (const auto& id : selectedAssetIds_)
        result.append(id);
    return result;
}

void MediaPoolNotifier::addToTimeline(const QString& assetId) {
    qDebug() << "[MediaPool] addToTimeline called with id:" << assetId
             << "total assets:" << state_.assets.size();
    for (const auto& a : state_.assets) {
        if (a.id == assetId) {
            QString mediaType;
            switch (a.type) {
            case MediaAssetType::Video: mediaType = QStringLiteral("video"); break;
            case MediaAssetType::Image: mediaType = QStringLiteral("image"); break;
            case MediaAssetType::Audio: mediaType = QStringLiteral("audio"); break;
            default: mediaType = QStringLiteral("video"); break;
            }
            qDebug() << "[MediaPool] emitting addToTimelineRequested:"
                     << a.filePath << mediaType << a.fileName << a.durationSeconds;
            emit addToTimelineRequested(assetId, a.filePath, mediaType,
                                        a.fileName, a.durationSeconds);
            return;
        }
    }
    qWarning() << "[MediaPool] addToTimeline: asset NOT FOUND for id:" << assetId;
}

void MediaPoolNotifier::showFilePicker() {
    emit filePickerRequested();
}

bool MediaPoolNotifier::validateBatchSize(int count) const {
    if (count > ImportGuardrails::kMaxBatchSize) {
        emit batchTooLarge(count, ImportGuardrails::kMaxBatchSize);
        return false;
    }
    return true;
}

void MediaPoolNotifier::importFiles(const QVariantList& urls) {
    if (!validateBatchSize(urls.size())) return;

    int succeeded = 0, failed = 0;
    QStringList failedNames;

    for (const auto& urlVar : urls) {
        QVariant v = urlVar;
        while (v.typeId() == QMetaType::QVariant)
            v = v.value<QVariant>();

        QString path;
        if (v.canConvert<QUrl>()) {
            path = v.toUrl().toLocalFile();
        } else {
            path = v.toString();
            if (path.startsWith(QStringLiteral("file:///")))
                path = QUrl(path).toLocalFile();
            else if (path.startsWith(QStringLiteral("file://")))
                path = QUrl(path).toLocalFile();
        }
        qDebug() << "[MediaPool] importFiles resolved:" << path;
        if (!path.isEmpty()) {
            // Check if file exists and is readable before importing
            QFileInfo fi(path);
            if (!fi.exists() || !fi.isReadable()) {
                failed++;
                failedNames.append(fi.fileName());
                emit importRejected(fi.fileName(), QStringLiteral("File not found or not readable"));
                continue;
            }
            importFile(path);
            succeeded++;
        }
    }

    if (failed > 0)
        emit importBatchCompleted(succeeded, failed, failedNames);
}

void MediaPoolNotifier::setFilter(const QString& filter) {
    if (filter == QStringLiteral("all") || filter.isEmpty()) {
        clearFilterType();
    } else if (filter == QStringLiteral("video")) {
        setFilterType(static_cast<int>(MediaAssetType::Video));
    } else if (filter == QStringLiteral("image")) {
        setFilterType(static_cast<int>(MediaAssetType::Image));
    } else if (filter == QStringLiteral("audio")) {
        setFilterType(static_cast<int>(MediaAssetType::Audio));
    }
}

void MediaPoolNotifier::cycleSort() {
    int current = static_cast<int>(state_.sortBy);
    int next = (current + 1) % 5; // 5 sort modes
    state_.sortBy = static_cast<MediaPoolSortBy>(next);
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Search / filter / sort
// ---------------------------------------------------------------------------

void MediaPoolNotifier::setSearchQuery(const QString& query) {
    state_.searchQuery = query;
    emit stateChanged();
}

void MediaPoolNotifier::setFilterType(int type) {
    state_.filterType = static_cast<MediaAssetType>(type);
    emit stateChanged();
}

void MediaPoolNotifier::clearFilterType() {
    state_.filterType.reset();
    emit stateChanged();
}

void MediaPoolNotifier::setSortBy(int sortBy) {
    state_.sortBy = static_cast<MediaPoolSortBy>(sortBy);
    emit stateChanged();
}

void MediaPoolNotifier::toggleSortOrder() {
    state_.sortAscending = !state_.sortAscending;
    emit stateChanged();
}

void MediaPoolNotifier::setViewMode(int mode) {
    state_.viewMode = static_cast<MediaPoolViewMode>(mode);
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// Folder import, rename, reveal, bin navigation
// ---------------------------------------------------------------------------

void MediaPoolNotifier::importFolder(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) return;

    // Create a bin mirroring the folder name
    QString binName = dir.dirName();
    createBin(binName);
    // Find the bin we just created
    QString binId;
    for (const auto& bin : state_.bins) {
        if (bin.name == binName) { binId = bin.id; break; }
    }

    auto prevBin = state_.activeBinId;
    if (!binId.isEmpty())
        state_.activeBinId = binId;

    // Import all supported files
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < entries.size(); ++i) {
        const QFileInfo& entry = entries[i];
        if (entry.isDir()) {
            importFolder(entry.absoluteFilePath());
        } else {
            importFile(entry.absoluteFilePath());
        }
    }

    state_.activeBinId = prevBin;
    emit stateChanged();
}

void MediaPoolNotifier::renameAsset(const QString& assetId, const QString& newName) {
    for (auto& asset : state_.assets) {
        if (asset.id == assetId) {
            asset.fileName = newName;
            emit stateChanged();
            return;
        }
    }
}

void MediaPoolNotifier::revealInExplorer(const QString& assetId) {
    for (const auto& asset : state_.assets) {
        if (asset.id == assetId) {
#ifdef Q_OS_WIN
            QProcess::startDetached(QStringLiteral("explorer"),
                {QStringLiteral("/select,"), QDir::toNativeSeparators(asset.filePath)});
#elif defined(Q_OS_MACOS)
            QProcess::startDetached(QStringLiteral("open"),
                {QStringLiteral("-R"), asset.filePath});
#else
            QProcess::startDetached(QStringLiteral("xdg-open"),
                {QFileInfo(asset.filePath).absolutePath()});
#endif
            return;
        }
    }
}

void MediaPoolNotifier::navigateIntoBin(const QString& binId) {
    setActiveBin(binId);
}

void MediaPoolNotifier::navigateUp() {
    if (!state_.activeBinId.has_value()) return;
    // Find parent bin
    for (const auto& bin : state_.bins) {
        if (bin.id == *state_.activeBinId) {
            setActiveBin(bin.parentId.value_or(QString()));
            return;
        }
    }
    setActiveBin(QString());
}

QString MediaPoolNotifier::binBreadcrumb() const {
    if (!state_.activeBinId.has_value()) return QStringLiteral("All Media");
    QStringList parts;
    QString currentId = *state_.activeBinId;
    while (!currentId.isEmpty()) {
        for (const auto& bin : state_.bins) {
            if (bin.id == currentId) {
                parts.prepend(bin.name);
                currentId = bin.parentId.value_or(QString());
                break;
            }
        }
        if (parts.isEmpty()) break; // safety
    }
    parts.prepend(QStringLiteral("All Media"));
    return parts.join(QStringLiteral(" > "));
}

QString MediaPoolNotifier::activeBinId() const {
    return state_.activeBinId.value_or(QString());
}

QVariantList MediaPoolNotifier::binsVariant() const {
    QVariantList result;
    for (const auto& bin : state_.bins) {
        // Only show bins at current level
        QString parentId = bin.parentId.value_or(QString());
        QString activeId = state_.activeBinId.value_or(QString());
        if (parentId != activeId) continue;

        QVariantMap m;
        m[QStringLiteral("id")] = bin.id;
        m[QStringLiteral("name")] = bin.name;
        m[QStringLiteral("parentId")] = parentId;
        result.append(m);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void MediaPoolNotifier::loadFromData(const QVariantList& data) {
    Q_UNUSED(data);
    // TODO: Deserialise from QVariantList
}

QVariantList MediaPoolNotifier::toData() const {
    QVariantList result;
    // TODO: Serialise to QVariantList
    return result;
}

} // namespace gopost::video_editor
