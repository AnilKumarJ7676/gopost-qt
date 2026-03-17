#if defined(__APPLE__)
#include "decoder_interface.hpp"
#include "gopost/engine.h"
#include "gopost/error.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>

namespace gopost {
namespace video {

// ============================================================================
// Helper: rotation from AVAssetTrack preferredTransform
// ============================================================================

static Rotation rotation_from_transform(CGAffineTransform t) {
    if (t.a == 0 && t.b == 1.0 && t.c == -1.0 && t.d == 0)
        return Rotation::CW90;
    if (t.a == -1.0 && t.b == 0 && t.c == 0 && t.d == -1.0)
        return Rotation::CW180;
    if (t.a == 0 && t.b == -1.0 && t.c == 1.0 && t.d == 0)
        return Rotation::CW270;
    return Rotation::None;
}

// ============================================================================
// AVFoundationVideoDecoder
// ============================================================================

class AVFoundationVideoDecoder : public IVideoDecoder {
public:
    explicit AVFoundationVideoDecoder(GopostEngine* engine) : engine_(engine) {}

    ~AVFoundationVideoDecoder() override { close(); }

    bool open(const std::string& path) override {
        @autoreleasepool {
            close();
            if (path.empty() || !engine_) return false;

            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
            asset_ = [AVURLAsset URLAssetWithURL:url options:nil];
            if (!asset_) return false;

            NSArray<AVAssetTrack*>* tracks = [asset_ tracksWithMediaType:AVMediaTypeVideo];
            if (tracks.count == 0) return false;

            AVAssetTrack* vt = tracks[0];
            CGSize size = vt.naturalSize;
            CGAffineTransform transform = vt.preferredTransform;

            info_.rotation = rotation_from_transform(transform);

            if (info_.rotation == Rotation::CW90 || info_.rotation == Rotation::CW270) {
                info_.width  = (int32_t)size.height;
                info_.height = (int32_t)size.width;
            } else {
                info_.width  = (int32_t)size.width;
                info_.height = (int32_t)size.height;
            }

            info_.frame_rate = vt.nominalFrameRate;
            if (info_.frame_rate <= 0) info_.frame_rate = 30.0;
            info_.duration_seconds = CMTimeGetSeconds(asset_.duration);
            info_.frame_count = (int64_t)(info_.duration_seconds * info_.frame_rate);

            // Codec name from format descriptions
            NSArray* descs = vt.formatDescriptions;
            if (descs.count > 0) {
                CMFormatDescriptionRef desc = (__bridge CMFormatDescriptionRef)descs[0];
                CMVideoCodecType codecType = CMFormatDescriptionGetMediaSubType(desc);
                switch (codecType) {
                    case kCMVideoCodecType_H264:       info_.codec_name = "h264"; break;
                    case kCMVideoCodecType_HEVC:       info_.codec_name = "hevc"; break;
                    case kCMVideoCodecType_MPEG4Video: info_.codec_name = "mpeg4"; break;
                    case kCMVideoCodecType_AppleProRes422:
                    case kCMVideoCodecType_AppleProRes4444:
                        info_.codec_name = "prores"; break;
                    default: {
                        char fourcc[5] = {};
                        fourcc[0] = (char)((codecType >> 24) & 0xFF);
                        fourcc[1] = (char)((codecType >> 16) & 0xFF);
                        fourcc[2] = (char)((codecType >>  8) & 0xFF);
                        fourcc[3] = (char)((codecType      ) & 0xFF);
                        info_.codec_name = fourcc;
                        break;
                    }
                }
            }

            info_.bitrate = static_cast<int64_t>(vt.estimatedDataRate);

            // Image generator for random-access decode
            imageGen_ = [[AVAssetImageGenerator alloc] initWithAsset:asset_];
            imageGen_.appliesPreferredTrackTransform = YES;
            imageGen_.requestedTimeToleranceBefore = CMTimeMake(1, 600);
            imageGen_.requestedTimeToleranceAfter = CMTimeMake(1, 600);
            imageGen_.maximumSize = CGSizeMake(info_.width, info_.height);

            current_time_ = 0;
            eof_ = false;
            path_ = path;
            return true;
        }
    }

    void close() override {
        @autoreleasepool {
            invalidate_reader();
            imageGen_ = nil;
            asset_ = nil;
            path_.clear();
            info_ = {};
            current_time_ = 0;
            eof_ = false;
        }
    }

    bool is_open() const override { return !path_.empty() && asset_ != nil; }
    VideoStreamInfo info() const override { return info_; }
    bool is_eof() const override { return eof_; }

    // --- random-access decode ---

    GopostFrame* decode_frame_at(double source_time_seconds) override {
        @autoreleasepool {
            if (!is_open() || !engine_) return nullptr;

            double clamped = source_time_seconds;
            if (clamped < 0) clamped = 0;
            if (clamped > info_.duration_seconds) clamped = info_.duration_seconds;

            CMTime requestTime = CMTimeMakeWithSeconds(clamped, 600);
            CMTime actualTime;
            NSError* error = nil;

            CGImageRef cgImage = [imageGen_ copyCGImageAtTime:requestTime
                                                   actualTime:&actualTime
                                                        error:&error];
            if (!cgImage) return nullptr;

            size_t w = CGImageGetWidth(cgImage);
            size_t h = CGImageGetHeight(cgImage);

            GopostFrame* frame = nullptr;
            if (gopost_frame_acquire(engine_, &frame, (uint32_t)w, (uint32_t)h,
                                     GOPOST_PIXEL_FORMAT_RGBA8) != GOPOST_OK) {
                CGImageRelease(cgImage);
                return nullptr;
            }
            if (!frame || !frame->data) {
                CGImageRelease(cgImage);
                return nullptr;
            }

            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef ctx = CGBitmapContextCreate(
                frame->data, w, h, 8, w * 4, colorSpace,
                kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);

            if (ctx) {
                CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cgImage);
                CGContextRelease(ctx);
            }
            CGColorSpaceRelease(colorSpace);
            CGImageRelease(cgImage);
            return frame;
        }
    }

    // --- sequential decode ---

    bool seek_to(double timestamp_seconds) override {
        @autoreleasepool {
            if (!is_open()) return false;
            double clamped = timestamp_seconds;
            if (clamped < 0) clamped = 0;
            if (clamped > info_.duration_seconds) clamped = info_.duration_seconds;
            current_time_ = clamped;
            eof_ = false;
            invalidate_reader();
            return true;
        }
    }

    std::optional<DecodedFrame> decode_next_frame() override {
        @autoreleasepool {
            if (!is_open() || eof_) return std::nullopt;

            if (!ensure_reader()) {
                eof_ = true;
                return std::nullopt;
            }

            if (reader_output_ == nil || reader_.status != AVAssetReaderStatusReading) {
                eof_ = true;
                return std::nullopt;
            }

            CMSampleBufferRef sampleBuffer = [reader_output_ copyNextSampleBuffer];
            if (!sampleBuffer) {
                eof_ = true;
                return std::nullopt;
            }

            CMTime pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
            double ts = CMTimeGetSeconds(pts);

            CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
            if (!imageBuffer) {
                CFRelease(sampleBuffer);
                eof_ = true;
                return std::nullopt;
            }

            CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

            size_t w = CVPixelBufferGetWidth(imageBuffer);
            size_t h = CVPixelBufferGetHeight(imageBuffer);
            size_t rowBytes = CVPixelBufferGetBytesPerRow(imageBuffer);
            uint8_t* baseAddr = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(imageBuffer));

            DecodedFrame df;
            df.width  = static_cast<uint32_t>(w);
            df.height = static_cast<uint32_t>(h);
            df.format = GOPOST_PIXEL_FORMAT_RGBA8;
            df.timestamp_seconds = ts;
            df.pts = static_cast<int64_t>(pts.value);

            const size_t dst_stride = w * 4;
            df.pixels.resize(dst_stride * h);

            // CVPixelBuffer is BGRA; convert to RGBA
            for (size_t y = 0; y < h; y++) {
                const uint8_t* src_row = baseAddr + y * rowBytes;
                uint8_t* dst_row = df.pixels.data() + y * dst_stride;
                for (size_t x = 0; x < w; x++) {
                    dst_row[x * 4 + 0] = src_row[x * 4 + 2]; // R
                    dst_row[x * 4 + 1] = src_row[x * 4 + 1]; // G
                    dst_row[x * 4 + 2] = src_row[x * 4 + 0]; // B
                    dst_row[x * 4 + 3] = src_row[x * 4 + 3]; // A
                }
            }

            CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
            CFRelease(sampleBuffer);

            current_time_ = ts + (1.0 / info_.frame_rate);
            return df;
        }
    }

private:
    bool ensure_reader() {
        if (reader_ != nil && reader_.status == AVAssetReaderStatusReading) {
            return true;
        }

        invalidate_reader();

        NSError* error = nil;
        reader_ = [[AVAssetReader alloc] initWithAsset:asset_ error:&error];
        if (!reader_ || error) return false;

        CMTime start = CMTimeMakeWithSeconds(current_time_, 600);
        CMTime duration = CMTimeSubtract(asset_.duration, start);
        if (CMTimeCompare(duration, kCMTimeZero) <= 0) return false;
        reader_.timeRange = CMTimeRangeMake(start, duration);

        NSArray<AVAssetTrack*>* tracks = [asset_ tracksWithMediaType:AVMediaTypeVideo];
        if (tracks.count == 0) return false;

        NSDictionary* outputSettings = @{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
        };
        reader_output_ = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:tracks[0]
                                                                   outputSettings:outputSettings];
        reader_output_.alwaysCopiesSampleData = NO;

        if (![reader_ canAddOutput:reader_output_]) return false;
        [reader_ addOutput:reader_output_];

        return [reader_ startReading];
    }

    void invalidate_reader() {
        @autoreleasepool {
            if (reader_ && reader_.status == AVAssetReaderStatusReading) {
                [reader_ cancelReading];
            }
            reader_output_ = nil;
            reader_ = nil;
        }
    }

    GopostEngine* engine_ = nullptr;
    std::string path_;
    VideoStreamInfo info_;
    double current_time_ = 0;
    bool eof_ = false;

    AVURLAsset* __strong asset_ = nil;
    AVAssetImageGenerator* __strong imageGen_ = nil;
    AVAssetReader* __strong reader_ = nil;
    AVAssetReaderTrackOutput* __strong reader_output_ = nil;
};

std::unique_ptr<IVideoDecoder> create_video_decoder(GopostEngine* engine) {
    return std::make_unique<AVFoundationVideoDecoder>(engine);
}

}  // namespace video
}  // namespace gopost
#endif
