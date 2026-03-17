#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

namespace gopost::video_editor {

/// QQuickImageProvider that serves rendered video frames to QML.
/// Thread-safe: engine writes frames via updateFrame(), QML reads via requestImage().
class VideoFrameProvider : public QQuickImageProvider {
public:
    VideoFrameProvider();

    /// Called by QML's Image element when source changes.
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    /// Called by TimelineNotifier on each playback tick with a new rendered frame.
    void updateFrame(const QImage& frame);

    /// Returns true if at least one frame has been rendered.
    bool hasFrame() const;

private:
    mutable QMutex mutex_;
    QImage currentFrame_;
};

} // namespace gopost::video_editor
