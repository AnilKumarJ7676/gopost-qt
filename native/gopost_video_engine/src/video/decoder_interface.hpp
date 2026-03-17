#ifndef GOPOST_VIDEO_DECODER_INTERFACE_HPP
#define GOPOST_VIDEO_DECODER_INTERFACE_HPP

#include "gopost/types.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct GopostEngine;

namespace gopost {
namespace video {

// ============================================================================
// Rotation
// ============================================================================

enum class Rotation : int {
    None   = 0,
    CW90   = 90,
    CW180  = 180,
    CW270  = 270,
};

// ============================================================================
// VideoStreamInfo  (was VideoMetadata in the spec)
// ============================================================================

struct VideoStreamInfo {
    int32_t  width            = 0;
    int32_t  height           = 0;
    double   frame_rate       = 30.0;
    double   duration_seconds = 0;
    int64_t  frame_count      = 0;

    // --- newly added fields ---
    std::string codec_name;            // e.g. "h264", "hevc", "vp9"
    int64_t     bitrate      = 0;      // bits per second (0 = unknown)
    Rotation    rotation     = Rotation::None;
};

// ============================================================================
// DecodedFrame — owning, self-contained decoded video frame
// ============================================================================

struct DecodedFrame {
    std::vector<uint8_t> pixels;       // RGBA8 pixel data (owned)
    uint32_t             width  = 0;
    uint32_t             height = 0;
    GopostPixelFormat    format = GOPOST_PIXEL_FORMAT_RGBA8;
    double               timestamp_seconds = 0.0;
    int64_t              pts    = 0;   // presentation timestamp in stream timebase
};

// ============================================================================
// IVideoDecoder — abstract video decoder
// ============================================================================

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    // --- lifecycle ---
    virtual bool open(const std::string& path) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    // --- metadata ---
    virtual VideoStreamInfo info() const = 0;

    // --- random-access decode (existing) ---
    /**
     * Decode frame at source time (seconds). Returns frame from engine pool;
     * caller must gopost_frame_release(). Returns nullptr on error.
     */
    virtual GopostFrame* decode_frame_at(double source_time_seconds) = 0;

    // --- sequential decode (newly added) ---

    /** Seek to the given timestamp. Returns true on success. */
    virtual bool seek_to(double timestamp_seconds) = 0;

    /**
     * Decode the next frame sequentially from the current position.
     * Returns std::nullopt on error or end-of-stream.
     */
    virtual std::optional<DecodedFrame> decode_next_frame() = 0;

    /** True if the decoder has reached the end of the stream. */
    virtual bool is_eof() const = 0;
};

/** Factory: returns a decoder instance (e.g. stub or FFmpeg). Engine used for frame pool. */
std::unique_ptr<IVideoDecoder> create_video_decoder(GopostEngine* engine);

}  // namespace video
}  // namespace gopost

#endif
