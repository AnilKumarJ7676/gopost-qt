#pragma once

#include <memory>
#include <QDebug>

#include "video_editor/data/services/native_thumbnail_service.h"

namespace gopost::video_editor {

/// Manages the native DecoderPool lifecycle and provides the
/// NativeThumbnailService to the rest of the app.
///
/// Usage:
/// 1. Create after engine initialization with the native lib handle and engine pointer.
/// 2. Call initialize() to create the decoder pool.
/// 3. Use thumbnailService() for thumbnail extraction.
/// 4. Call dispose() when the editor is closed.
class DecoderPoolManager {
public:
    DecoderPoolManager(void* libHandle, void* enginePtr);
    ~DecoderPoolManager();

    /// The native thumbnail service. Available after initialize().
    NativeThumbnailService* thumbnailService() { return thumbnailService_.get(); }

    /// Whether the decoder pool is initialized.
    bool isInitialized() const { return initialized_; }

    /// Initialize the decoder pool and thumbnail generator.
    /// maxDecoders controls how many video decoders can be open simultaneously.
    /// - Desktop (macOS, Windows, Linux): 2-3 is safe.
    /// - Mobile (iOS, Android): 1-2 depending on device capabilities.
    void initialize(int maxDecoders = 0);

    /// Reduce max decoders for low-memory situations.
    void reduceDecoders();

    /// Flush idle decoders to reclaim memory.
    void flushIdle();

    /// Dispose all native resources.
    void dispose();

private:
    void* libHandle_{nullptr};
    void* enginePtr_{nullptr};
    std::unique_ptr<NativeThumbnailService> thumbnailService_;
    bool initialized_{false};

    static int defaultMaxDecoders();
};

} // namespace gopost::video_editor
