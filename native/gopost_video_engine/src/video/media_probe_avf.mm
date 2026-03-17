#if defined(__APPLE__)
#include "media_probe.hpp"
#import <AVFoundation/AVFoundation.h>

namespace gopost {
namespace video {

bool probe_media(const std::string& path, MediaProbeInfo& out) {
    @autoreleasepool {
        if (path.empty()) return false;

        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
        AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];
        if (!asset) return false;

        out.duration_seconds = CMTimeGetSeconds(asset.duration);
        if (out.duration_seconds <= 0 && out.duration_seconds != 0) {
            return false;
        }

        NSArray<AVAssetTrack*>* videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];
        if (videoTracks.count > 0) {
            AVAssetTrack* vt = videoTracks[0];
            CGSize size = vt.naturalSize;
            CGAffineTransform transform = vt.preferredTransform;

            // Account for rotation (portrait video has a 90-degree transform).
            if (transform.b == 1.0 && transform.c == -1.0) {
                out.width = (int32_t)size.height;
                out.height = (int32_t)size.width;
            } else if (transform.b == -1.0 && transform.c == 1.0) {
                out.width = (int32_t)size.height;
                out.height = (int32_t)size.width;
            } else {
                out.width = (int32_t)size.width;
                out.height = (int32_t)size.height;
            }

            out.frame_rate = vt.nominalFrameRate;
            if (out.frame_rate <= 0) out.frame_rate = 30.0;
            out.frame_count = (int64_t)(out.duration_seconds * out.frame_rate);
        }

        NSArray<AVAssetTrack*>* audioTracks = [asset tracksWithMediaType:AVMediaTypeAudio];
        if (audioTracks.count > 0) {
            out.has_audio = true;
            AVAssetTrack* at = audioTracks[0];
            out.audio_duration_seconds = CMTimeGetSeconds(at.timeRange.duration);
            if (out.audio_duration_seconds <= 0) {
                out.audio_duration_seconds = out.duration_seconds;
            }

            const AudioStreamBasicDescription* desc = CMAudioFormatDescriptionGetStreamBasicDescription(
                (__bridge CMAudioFormatDescriptionRef)at.formatDescriptions.firstObject);
            if (desc) {
                out.audio_sample_rate = (int32_t)desc->mSampleRate;
                out.audio_channels = (int32_t)desc->mChannelsPerFrame;
            } else {
                out.audio_sample_rate = 44100;
                out.audio_channels = 2;
            }
        }

        return true;
    }
}

}  // namespace video
}  // namespace gopost
#endif
