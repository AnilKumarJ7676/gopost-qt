#ifndef GOPOST_VIDEO_COMPOSITOR_HPP
#define GOPOST_VIDEO_COMPOSITOR_HPP

#include "gopost/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Blend mode for video (matches canvas blend 0-4). */
typedef enum {
    GOPOST_VIDEO_BLEND_NORMAL   = 0,
    GOPOST_VIDEO_BLEND_MULTIPLY  = 1,
    GOPOST_VIDEO_BLEND_SCREEN   = 2,
    GOPOST_VIDEO_BLEND_OVERLAY  = 3,
    GOPOST_VIDEO_BLEND_ADD      = 4,
} GopostVideoBlendMode;

/**
 * Composite overlay onto base in-place (base = base blended with overlay).
 * Both frames must be RGBA8, same dimensions. overlay opacity 0..1.
 */
int gopost_video_composite_blend(
    GopostFrame* base,
    const GopostFrame* overlay,
    float overlay_opacity,
    GopostVideoBlendMode mode);

/**
 * Copy source into dest (same size, RGBA8). Optional: clear dest first.
 */
int gopost_video_frame_copy(GopostFrame* dest, const GopostFrame* source, int clear_dest_first);

/**
 * Clear frame to transparent black (0,0,0,0).
 */
int gopost_video_frame_clear(GopostFrame* frame);

#ifdef __cplusplus
}
#endif

#endif
