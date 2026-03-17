#pragma once

#include <QString>
#include <QByteArray>
#include <QList>
#include <optional>

namespace gopost::video_editor {

/// DIP: Abstraction for thumbnail extraction so callers don't depend on
/// the concrete FFmpeg-backed implementation.
class ThumbnailServiceInterface {
public:
    virtual ~ThumbnailServiceInterface() = default;

    virtual QList<QByteArray> getCached(const QString& sourcePath, int count) = 0;

    virtual QList<QByteArray> extractThumbnails(
        const QString& sourcePath,
        double sourceDuration,
        int count
    ) = 0;

    virtual std::optional<QByteArray> extractSingleThumbnail(
        const QString& sourcePath,
        double timeSeconds = 0.5
    ) = 0;

    virtual void clearCache() = 0;
};

} // namespace gopost::video_editor
