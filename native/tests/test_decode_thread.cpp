#include <gtest/gtest.h>
#include "video/decode_thread.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

using gopost::video::DecodedFrame;
using gopost::video::DecodeThread;
using gopost::video::IVideoDecoder;
using gopost::video::RingFrameBuffer;
using gopost::video::Rotation;
using gopost::video::VideoStreamInfo;

// ============================================================================
// Mock decoder for testing — generates synthetic frames sequentially
// ============================================================================

class MockSequentialDecoder : public IVideoDecoder {
public:
    MockSequentialDecoder(int total_frames = 100, double fps = 30.0)
        : total_frames_(total_frames), fps_(fps) {}

    bool open(const std::string& path) override {
        path_ = path;
        open_ = true;
        pos_ = 0;
        return !path.empty();
    }

    void close() override {
        open_ = false;
        pos_ = 0;
    }

    bool is_open() const override { return open_; }

    VideoStreamInfo info() const override {
        VideoStreamInfo si;
        si.width = 8;
        si.height = 8;
        si.frame_rate = fps_;
        si.duration_seconds = total_frames_ / fps_;
        si.frame_count = total_frames_;
        return si;
    }

    GopostFrame* decode_frame_at(double) override { return nullptr; }

    bool seek_to(double timestamp_seconds) override {
        if (!open_) return false;
        pos_ = static_cast<int>(timestamp_seconds * fps_);
        if (pos_ < 0) pos_ = 0;
        if (pos_ > total_frames_) pos_ = total_frames_;
        seek_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    std::optional<DecodedFrame> decode_next_frame() override {
        if (!open_ || pos_ >= total_frames_) return std::nullopt;

        // Simulate decode latency
        if (decode_delay_us_ > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(decode_delay_us_));
        }

        // Optionally inject errors
        if (error_at_frame_ >= 0 && pos_ == error_at_frame_) {
            pos_++;
            return std::nullopt;
        }

        DecodedFrame f;
        f.width = 8;
        f.height = 8;
        f.timestamp_seconds = pos_ / fps_;
        f.pts = pos_;
        f.pixels.assign(8 * 8 * 4, static_cast<uint8_t>(pos_ & 0xFF));

        pos_++;
        frames_produced_.fetch_add(1, std::memory_order_relaxed);
        return f;
    }

    bool is_eof() const override { return open_ && pos_ >= total_frames_; }

    // Test controls
    void set_decode_delay_us(int us) { decode_delay_us_ = us; }
    void set_error_at_frame(int frame) { error_at_frame_ = frame; }
    int position() const { return pos_; }
    int seek_count() const { return seek_count_.load(std::memory_order_relaxed); }
    int frames_produced() const { return frames_produced_.load(std::memory_order_relaxed); }

private:
    std::string path_;
    bool open_ = false;
    int pos_ = 0;
    int total_frames_;
    double fps_;
    int decode_delay_us_ = 0;
    int error_at_frame_ = -1;
    std::atomic<int> seek_count_{0};
    std::atomic<int> frames_produced_{0};
};

// ============================================================================
// Tests
// ============================================================================

TEST(DecodeThreadTest, StartRequiresOpenDecoder) {
    auto decoder = std::make_unique<MockSequentialDecoder>();
    RingFrameBuffer ring(8);

    DecodeThread dt(std::move(decoder), ring);

    // start() without open() should fail
    EXPECT_FALSE(dt.start());
    EXPECT_FALSE(dt.is_running());
}

TEST(DecodeThreadTest, OpenAndStart) {
    auto decoder = std::make_unique<MockSequentialDecoder>(50);
    RingFrameBuffer ring(8);

    DecodeThread dt(std::move(decoder), ring);
    ASSERT_TRUE(dt.open("test_video.mp4"));
    ASSERT_TRUE(dt.start());
    EXPECT_TRUE(dt.is_running());

    // Let it decode some frames
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    dt.stop();
    EXPECT_FALSE(dt.is_running());
    EXPECT_GT(ring.size(), 0u);
}

TEST(DecodeThreadTest, StopIsIdempotent) {
    auto decoder = std::make_unique<MockSequentialDecoder>(10);
    RingFrameBuffer ring(8);

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();
    dt.stop();
    dt.stop();  // Should not crash or hang
    EXPECT_FALSE(dt.is_running());
}

TEST(DecodeThreadTest, DestructorCallsStop) {
    RingFrameBuffer ring(8);
    {
        auto decoder = std::make_unique<MockSequentialDecoder>(100);
        DecodeThread dt(std::move(decoder), ring);
        dt.open("test.mp4");
        dt.start();
        // dt destructor should call stop() and join
    }
    // If we get here without hanging, the test passes
    SUCCEED();
}

TEST(DecodeThreadTest, DecodesAllFramesToEOF) {
    constexpr int kTotalFrames = 20;
    auto decoder = std::make_unique<MockSequentialDecoder>(kTotalFrames);
    RingFrameBuffer ring(32);  // Larger than total frames so nothing blocks

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();

    // Wait for EOF
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(dt.is_eof());
    EXPECT_EQ(dt.frames_decoded(), kTotalFrames);

    // Drain the ring and verify FIFO ordering
    for (int i = 0; i < kTotalFrames; ++i) {
        auto f = ring.try_pop();
        ASSERT_TRUE(f.has_value()) << "Missing frame " << i;
        EXPECT_EQ(f->pts, i);
    }
    EXPECT_TRUE(ring.empty());

    dt.stop();
}

TEST(DecodeThreadTest, RingFullBackoff) {
    // Small ring, many frames — the thread should back off when full
    constexpr int kTotalFrames = 100;
    auto decoder = std::make_unique<MockSequentialDecoder>(kTotalFrames);
    RingFrameBuffer ring(4);

    DecodeThread dt(std::move(decoder), ring);
    dt.set_backoff_us(100);  // 100us backoff
    dt.open("test.mp4");
    dt.start();

    // Consume frames one by one, letting the producer keep up
    int consumed = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (consumed < kTotalFrames && std::chrono::steady_clock::now() < deadline) {
        auto f = ring.try_pop();
        if (f.has_value()) {
            EXPECT_EQ(f->pts, consumed);
            consumed++;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }

    EXPECT_EQ(consumed, kTotalFrames);
    dt.stop();
}

TEST(DecodeThreadTest, SeekClearsRingAndResumes) {
    constexpr int kTotalFrames = 100;
    constexpr double kFps = 30.0;
    auto* raw_decoder = new MockSequentialDecoder(kTotalFrames, kFps);
    auto decoder = std::unique_ptr<IVideoDecoder>(raw_decoder);
    RingFrameBuffer ring(16);

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();

    // Let it decode some frames
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Drain whatever is in the ring
    while (ring.try_pop().has_value()) {}

    // Seek to frame 50 (≈ 1.667s at 30fps)
    double seek_time = 50.0 / kFps;
    dt.seek_to(seek_time);

    // Wait for frames to appear after seek
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    std::optional<DecodedFrame> first_after_seek;
    while (!first_after_seek.has_value() &&
           std::chrono::steady_clock::now() < deadline) {
        first_after_seek = ring.try_pop();
        if (!first_after_seek.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    ASSERT_TRUE(first_after_seek.has_value());
    // The first frame after seek should be at or near the target
    EXPECT_GE(first_after_seek->pts, 49);  // Allow ±1 frame tolerance

    EXPECT_GE(raw_decoder->seek_count(), 1);

    dt.stop();
}

TEST(DecodeThreadTest, SeekResetsEOF) {
    constexpr int kTotalFrames = 10;
    auto decoder = std::make_unique<MockSequentialDecoder>(kTotalFrames);
    RingFrameBuffer ring(32);

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();

    // Wait for EOF
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(dt.is_eof());

    // Seek back to start — should reset EOF
    dt.seek_to(0.0);

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_FALSE(dt.is_eof());

    // Should produce more frames
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        ring.try_pop();  // Drain to let decode proceed
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(dt.is_eof());

    dt.stop();
}

TEST(DecodeThreadTest, FrameDropCountOnDecodeError) {
    auto decoder = std::make_unique<MockSequentialDecoder>(20);
    decoder->set_error_at_frame(5);  // Frame 5 will fail
    RingFrameBuffer ring(32);

    std::atomic<int> warnings{0};
    DecodeThread dt(std::move(decoder), ring);
    dt.set_error_callback([&](DecodeThread::ErrorSeverity sev, const std::string&) {
        if (sev == DecodeThread::ErrorSeverity::Warning) {
            warnings.fetch_add(1, std::memory_order_relaxed);
        }
    });

    dt.open("test.mp4");
    dt.start();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(dt.is_eof());
    EXPECT_EQ(dt.frames_decoded(), 19u);   // 20 - 1 error
    EXPECT_EQ(dt.frames_dropped(), 1u);
    EXPECT_GE(warnings.load(), 1);

    dt.stop();
}

TEST(DecodeThreadTest, OpenFailReportsError) {
    auto decoder = std::make_unique<MockSequentialDecoder>();
    RingFrameBuffer ring(8);

    std::atomic<int> fatal_errors{0};
    DecodeThread dt(std::move(decoder), ring);
    dt.set_error_callback([&](DecodeThread::ErrorSeverity sev, const std::string&) {
        if (sev == DecodeThread::ErrorSeverity::Fatal) {
            fatal_errors.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Empty path causes mock decoder to fail
    EXPECT_FALSE(dt.open(""));
    EXPECT_GE(fatal_errors.load(), 1);
}

TEST(DecodeThreadTest, MultipleSeeksInSuccession) {
    constexpr int kTotalFrames = 200;
    constexpr double kFps = 30.0;
    auto decoder = std::make_unique<MockSequentialDecoder>(kTotalFrames, kFps);
    RingFrameBuffer ring(8);

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();

    // Rapid-fire seeks (simulating scrubbing)
    for (int i = 0; i < 10; ++i) {
        dt.seek_to(i * 0.5);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // After rapid seeks, thread should still be running and not deadlocked
    EXPECT_TRUE(dt.is_running());

    // Drain some frames to verify we get data
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    int got = 0;
    while (ring.try_pop().has_value()) got++;
    EXPECT_GT(got, 0);

    dt.stop();
}

TEST(DecodeThreadTest, FrameCounterAccuracy) {
    constexpr int kTotalFrames = 15;
    auto decoder = std::make_unique<MockSequentialDecoder>(kTotalFrames);
    RingFrameBuffer ring(32);

    DecodeThread dt(std::move(decoder), ring);
    dt.open("test.mp4");
    dt.start();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!dt.is_eof() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(dt.frames_decoded(), static_cast<uint64_t>(kTotalFrames));
    EXPECT_EQ(dt.frames_dropped(), 0u);
    dt.stop();
}
