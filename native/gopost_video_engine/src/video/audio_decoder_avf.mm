#if defined(__APPLE__)
#include "audio_decoder_interface.hpp"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

namespace gopost {
namespace video {

class AVFoundationAudioDecoder : public IAudioDecoder {
public:
    bool open(const std::string& path) override {
        @autoreleasepool {
            close();
            if (path.empty()) return false;

            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
            asset_ = [AVURLAsset URLAssetWithURL:url options:nil];
            if (!asset_) return false;

            NSArray<AVAssetTrack*>* audioTracks = [asset_ tracksWithMediaType:AVMediaTypeAudio];
            if (audioTracks.count == 0) {
                asset_ = nil;
                return false;
            }

            audioTrack_ = audioTracks[0];
            info_.duration_seconds = CMTimeGetSeconds(asset_.duration);

            const AudioStreamBasicDescription* desc =
                CMAudioFormatDescriptionGetStreamBasicDescription(
                    (__bridge CMAudioFormatDescriptionRef)audioTrack_.formatDescriptions.firstObject);
            if (desc) {
                info_.sample_rate = (int32_t)desc->mSampleRate;
                info_.channels = (int32_t)desc->mChannelsPerFrame;
            } else {
                info_.sample_rate = 44100;
                info_.channels = 2;
            }
            info_.total_samples = (int64_t)(info_.duration_seconds * info_.sample_rate);

            path_ = path;
            return true;
        }
    }

    void close() override {
        @autoreleasepool {
            audioTrack_ = nil;
            asset_ = nil;
            path_.clear();
            info_ = {};
        }
    }

    bool is_open() const override { return !path_.empty() && asset_ != nil; }
    AudioStreamInfo info() const override { return info_; }

    int32_t decode_audio_at(double time_seconds, float* buffer, int32_t max_frames,
                            int32_t target_sample_rate) override {
        @autoreleasepool {
            if (!is_open() || !buffer || max_frames <= 0) return 0;

            double clamped = time_seconds;
            if (clamped < 0) clamped = 0;
            if (clamped >= info_.duration_seconds) return 0;

            NSError* error = nil;
            AVAssetReader* reader = [[AVAssetReader alloc] initWithAsset:asset_ error:&error];
            if (!reader || error) return 0;

            NSDictionary* outputSettings = @{
                AVFormatIDKey: @(kAudioFormatLinearPCM),
                AVLinearPCMBitDepthKey: @32,
                AVLinearPCMIsFloatKey: @YES,
                AVLinearPCMIsNonInterleaved: @NO,
                AVSampleRateKey: @(target_sample_rate),
                AVNumberOfChannelsKey: @2,
            };

            AVAssetReaderTrackOutput* output =
                [[AVAssetReaderTrackOutput alloc] initWithTrack:audioTrack_
                                                outputSettings:outputSettings];
            [reader addOutput:output];

            CMTime startTime = CMTimeMakeWithSeconds(clamped, target_sample_rate);
            double endTime = clamped + (double)max_frames / (double)target_sample_rate;
            if (endTime > info_.duration_seconds) endTime = info_.duration_seconds;
            CMTime duration = CMTimeMakeWithSeconds(endTime - clamped, target_sample_rate);
            reader.timeRange = CMTimeRangeMake(startTime, duration);

            if (![reader startReading]) return 0;

            int32_t framesWritten = 0;
            const int32_t channels = 2;

            while (framesWritten < max_frames) {
                CMSampleBufferRef sampleBuffer = [output copyNextSampleBuffer];
                if (!sampleBuffer) break;

                CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
                if (!blockBuffer) {
                    CFRelease(sampleBuffer);
                    continue;
                }

                size_t length = 0;
                char* dataPtr = nullptr;
                OSStatus status = CMBlockBufferGetDataPointer(blockBuffer, 0, nullptr, &length, &dataPtr);
                if (status != noErr || !dataPtr) {
                    CFRelease(sampleBuffer);
                    continue;
                }

                int32_t sampleFrames = (int32_t)(length / (sizeof(float) * channels));
                int32_t framesToCopy = sampleFrames;
                if (framesWritten + framesToCopy > max_frames) {
                    framesToCopy = max_frames - framesWritten;
                }

                memcpy(buffer + framesWritten * channels,
                       dataPtr, framesToCopy * channels * sizeof(float));
                framesWritten += framesToCopy;

                CFRelease(sampleBuffer);
            }

            [reader cancelReading];
            return framesWritten;
        }
    }

private:
    std::string path_;
    AudioStreamInfo info_;
    AVURLAsset* __strong asset_ = nil;
    AVAssetTrack* __strong audioTrack_ = nil;
};

std::unique_ptr<IAudioDecoder> create_audio_decoder() {
    return std::make_unique<AVFoundationAudioDecoder>();
}

}  // namespace video
}  // namespace gopost
#endif
