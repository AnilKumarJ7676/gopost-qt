#include "video_editor/presentation/notifiers/video_frame_provider.h"

#include <QDebug>

namespace gopost::video_editor {

VideoFrameProvider::VideoFrameProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage VideoFrameProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    Q_UNUSED(id)
    QMutexLocker lock(&mutex_);

    if (currentFrame_.isNull()) {
        // Return a dark placeholder frame
        QSize sz = requestedSize.isValid() ? requestedSize : QSize(640, 360);
        QImage placeholder(sz, QImage::Format_RGB32);
        placeholder.fill(QColor(13, 13, 26));  // #0D0D1A
        if (size) *size = placeholder.size();
        return placeholder;
    }

    if (size) *size = currentFrame_.size();

    if (requestedSize.isValid() && requestedSize != currentFrame_.size()) {
        return currentFrame_.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return currentFrame_;
}

void VideoFrameProvider::updateFrame(const QImage& frame) {
    QMutexLocker lock(&mutex_);
    currentFrame_ = frame;
}

bool VideoFrameProvider::hasFrame() const {
    QMutexLocker lock(&mutex_);
    return !currentFrame_.isNull();
}

} // namespace gopost::video_editor
