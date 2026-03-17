#ifndef GOPOST_TEXTURE_BRIDGE_H
#define GOPOST_TEXTURE_BRIDGE_H

#include "gopost/types.h"
#include "gopost/error.h"
#include "gopost/engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque texture bridge handle — one per preview surface.
/// Manages a platform-native texture that Flutter renders via Texture widget.
typedef struct GopostTextureBridge GopostTextureBridge;

/// Hardware decoder capability info (returned by gopost_query_hw_decoder).
typedef struct {
    int32_t available;        /* 0 = no hw decoder, 1 = available */
    char device_name[128];    /* e.g. "d3d11va", "videotoolbox", "vaapi" */
    int32_t max_width;        /* max decode width (0 = unknown) */
    int32_t max_height;       /* max decode height (0 = unknown) */
} GopostHwDecoderInfo;

/// Query hardware decoder availability. Safe to call before timeline creation.
GopostError gopost_query_hw_decoder(GopostHwDecoderInfo* out_info);

/// Create a texture bridge that provides a Flutter texture ID.
///
/// The returned texture_id can be used in Dart with `Texture(textureId: id)`.
/// The bridge internally allocates a double-buffered pixel buffer of the
/// given dimensions.
///
/// @param engine       Active engine (needed for frame pool access).
/// @param width        Initial texture width in pixels.
/// @param height       Initial texture height in pixels.
/// @param out_bridge   Receives the opaque bridge handle.
/// @param out_texture_id Receives the Flutter texture ID (int64_t).
GopostError gopost_texture_bridge_create(
    GopostEngine* engine,
    int32_t width, int32_t height,
    GopostTextureBridge** out_bridge,
    int64_t* out_texture_id);

/// Push a new RGBA frame to the texture bridge.
///
/// The frame data is copied into the back buffer and the buffers are swapped.
/// Flutter is signalled to repaint on its next vsync.
/// The frame is NOT consumed — caller retains ownership (and should release
/// it to the frame pool when done).
GopostError gopost_texture_bridge_update_frame(
    GopostTextureBridge* bridge,
    const GopostFrame* frame);

/// Resize the texture bridge (e.g. when timeline resolution changes).
GopostError gopost_texture_bridge_resize(
    GopostTextureBridge* bridge,
    int32_t width, int32_t height);

/// Destroy and unregister the Flutter texture.
void gopost_texture_bridge_destroy(GopostTextureBridge* bridge);

#ifdef __cplusplus
}
#endif

#endif /* GOPOST_TEXTURE_BRIDGE_H */
