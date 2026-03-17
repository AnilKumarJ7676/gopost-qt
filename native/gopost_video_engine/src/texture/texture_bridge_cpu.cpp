/// Portable CPU-based texture bridge.
///
/// This implementation maintains a double-buffered RGBA pixel buffer.
/// Flutter integration (TextureRegistry) is handled on the Dart side:
/// the Dart FFI layer reads back the pixel buffer and feeds it to a
/// MemoryImage / RawImage widget, or to a platform-channel-based
/// TextureRegistry on platforms that support it.
///
/// This file is used on ALL platforms as the first working implementation.
/// Platform-specific zero-copy variants (D3D11, CVPixelBuffer, EGLImage)
/// can be added later and selected via CMake.

#include "gopost/texture_bridge.h"
#include "gopost/engine.h"

#include <cstring>
#include <mutex>
#include <vector>
#include <cstdint>
#include <algorithm>

#if defined(GOPOST_HAS_FFMPEG) && defined(GOPOST_USE_HW_DECODER)
#include "hw_video_decoder.hpp"
#endif

struct GopostTextureBridge {
    GopostEngine* engine = nullptr;
    int32_t width = 0;
    int32_t height = 0;

    // Double-buffered pixel data.
    // "front" is what Flutter reads; "back" is where new frames are written.
    std::mutex mutex;
    std::vector<uint8_t> buffer_a;
    std::vector<uint8_t> buffer_b;
    bool a_is_front = true;

    // Monotonically increasing frame counter — lets the Dart side detect
    // whether a new frame is available without polling the pixel data.
    int64_t frame_counter = 0;

    // Flutter texture ID assigned by the Dart-side TextureRegistry.
    // Set to -1 here; the Dart FFI layer assigns the real ID.
    int64_t texture_id = -1;
};

extern "C" {

// ============================================================================
// gopost_query_hw_decoder
// ============================================================================

GopostError gopost_query_hw_decoder(GopostHwDecoderInfo* out_info) {
    if (!out_info) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::memset(out_info, 0, sizeof(*out_info));

#if defined(GOPOST_HAS_FFMPEG) && defined(GOPOST_USE_HW_DECODER)
    auto caps = gopost::video::probe_hw_decoder_caps();
    out_info->available = caps.available ? 1 : 0;
    if (caps.available) {
        std::strncpy(out_info->device_name, caps.device_name.c_str(),
                      sizeof(out_info->device_name) - 1);
    }
#else
    out_info->available = 0;
#endif

    return GOPOST_OK;
}

// ============================================================================
// Texture bridge lifecycle
// ============================================================================

GopostError gopost_texture_bridge_create(
        GopostEngine* engine,
        int32_t width, int32_t height,
        GopostTextureBridge** out_bridge,
        int64_t* out_texture_id) {
    if (!engine || !out_bridge || !out_texture_id || width <= 0 || height <= 0) {
        return GOPOST_ERROR_INVALID_ARGUMENT;
    }

    auto* bridge = new (std::nothrow) GopostTextureBridge{};
    if (!bridge) return GOPOST_ERROR_OUT_OF_MEMORY;

    bridge->engine = engine;
    bridge->width = width;
    bridge->height = height;

    const size_t buf_size = static_cast<size_t>(width) * height * 4;
    bridge->buffer_a.resize(buf_size, 0);
    bridge->buffer_b.resize(buf_size, 0);

    // The CPU bridge doesn't register with Flutter's native TextureRegistry.
    // Instead, the Dart side creates a Texture via platform channels and
    // reads pixels back via FFI. We return a unique ID for tracking.
    static int64_t s_next_id = 1;
    bridge->texture_id = s_next_id++;

    *out_bridge = bridge;
    *out_texture_id = bridge->texture_id;
    return GOPOST_OK;
}

GopostError gopost_texture_bridge_update_frame(
        GopostTextureBridge* bridge,
        const GopostFrame* frame) {
    if (!bridge || !frame || !frame->data) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(bridge->mutex);

    // Write into the back buffer
    auto& back = bridge->a_is_front ? bridge->buffer_b : bridge->buffer_a;

    const size_t dst_row = static_cast<size_t>(bridge->width) * 4;
    const int32_t copy_h = std::min(bridge->height, static_cast<int32_t>(frame->height));
    const size_t copy_w = std::min(dst_row, frame->stride);

    for (int32_t y = 0; y < copy_h; y++) {
        std::memcpy(
            back.data() + y * dst_row,
            frame->data + y * frame->stride,
            copy_w);
    }

    // Swap front/back
    bridge->a_is_front = !bridge->a_is_front;
    bridge->frame_counter++;

    return GOPOST_OK;
}

GopostError gopost_texture_bridge_resize(
        GopostTextureBridge* bridge,
        int32_t width, int32_t height) {
    if (!bridge || width <= 0 || height <= 0) return GOPOST_ERROR_INVALID_ARGUMENT;

    std::lock_guard<std::mutex> lock(bridge->mutex);
    bridge->width = width;
    bridge->height = height;

    const size_t buf_size = static_cast<size_t>(width) * height * 4;
    bridge->buffer_a.assign(buf_size, 0);
    bridge->buffer_b.assign(buf_size, 0);

    return GOPOST_OK;
}

void gopost_texture_bridge_destroy(GopostTextureBridge* bridge) {
    delete bridge;
}

// ============================================================================
// Extra API for Dart FFI to read back the front buffer
// ============================================================================

/// Get a pointer to the current front buffer pixel data.
/// The returned pointer is valid until the next update_frame or destroy call.
/// Dart side copies this into an Image or Uint8List for display.
GopostError gopost_texture_bridge_get_pixels(
        const GopostTextureBridge* bridge,
        const uint8_t** out_data,
        int32_t* out_width,
        int32_t* out_height,
        int64_t* out_frame_counter) {
    if (!bridge || !out_data || !out_width || !out_height)
        return GOPOST_ERROR_INVALID_ARGUMENT;

    // No lock needed for atomic read of front buffer pointer — the double
    // buffer design ensures the front buffer is stable while back is written.
    const auto& front = bridge->a_is_front ? bridge->buffer_a : bridge->buffer_b;
    *out_data = front.data();
    *out_width = bridge->width;
    *out_height = bridge->height;
    if (out_frame_counter) *out_frame_counter = bridge->frame_counter;

    return GOPOST_OK;
}

}  // extern "C"
