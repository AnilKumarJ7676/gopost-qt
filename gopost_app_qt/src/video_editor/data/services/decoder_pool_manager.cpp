#include "video_editor/data/services/decoder_pool_manager.h"
#include "core/platform/platform_defaults.h"

#include <QSysInfo>
#include <QDebug>

namespace gopost::video_editor {

DecoderPoolManager::DecoderPoolManager(void* libHandle, void* enginePtr)
    : libHandle_(libHandle), enginePtr_(enginePtr) {}

DecoderPoolManager::~DecoderPoolManager() {
    dispose();
}

void DecoderPoolManager::initialize(int maxDecoders) {
    if (initialized_) return;

    const int effectiveMax = (maxDecoders > 0) ? maxDecoders : defaultMaxDecoders();

    thumbnailService_ = std::make_unique<NativeThumbnailService>(
        libHandle_, enginePtr_, effectiveMax);
    thumbnailService_->initialize();
    initialized_ = thumbnailService_->isInitialized();

    if (initialized_) {
        qDebug() << "[DecoderPool] Initialized with max" << effectiveMax << "decoders";
    } else {
        qDebug() << "[DecoderPool] Failed to initialize -- falling back to FFmpeg CLI";
    }
}

void DecoderPoolManager::reduceDecoders() {
    if (thumbnailService_)
        thumbnailService_->setMaxDecoders(1);
}

void DecoderPoolManager::flushIdle() {
    if (thumbnailService_)
        thumbnailService_->flushIdleDecoders();
}

void DecoderPoolManager::dispose() {
    thumbnailService_.reset();
    initialized_ = false;
}

int DecoderPoolManager::defaultMaxDecoders() {
    return platform::Defaults::forCurrentPlatform().maxDecoders;
}

} // namespace gopost::video_editor
