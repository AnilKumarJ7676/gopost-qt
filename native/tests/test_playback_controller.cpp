#include <gtest/gtest.h>
#include "video/playback_controller.hpp"

#include <chrono>
#include <cmath>
#include <thread>

using gopost::video::DecodedFrame;
using gopost::video::DecodeThread;
using gopost::video::IVideoDecoder;
using gopost::video::PlaybackController;
using gopost::video::PlaybackState;
using gopost::video::RingFrameBuffer;
using gopost::video::Rotation;
using gopost::video::VideoStreamInfo;

// ============================================================================
// Mock decoder — generates synthetic frames sequentially
// (Same as in test_decode_thread.cpp; duplicated here to keep test files
//  self-contained without shared test helpers.)
// ============================================================================

class MockDecoder : public IVideoDecoder {
public:
    MockDecoder(int total_frames = 100, double fps = 30.0)
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
        return true;
    }

    std::optional<DecodedFrame> decode_next_frame() override {
        if (!open_ || pos_ >= total_frames_) return std::nullopt;

        DecodedFrame f;
        f.width = 8;
        f.height = 8;
        f.timestamp_seconds = pos_ / fps_;
        f.pts = pos_;
        f.pixels.assign(8 * 8 * 4, static_cast<uint8_t>(pos_ & 0xFF));

        pos_++;
        return f;
    }

    bool is_eof() const override { return open_ && pos_ >= total_frames_; }

    int position() const { return pos_; }

private:
    std::string path_;
    bool open_ = false;
    int pos_ = 0;
    int total_frames_;
    double fps_;
};

// ============================================================================
// Test fixture — sets up decoder + ring + decode thread + controller
// ============================================================================

class PlaybackControllerTest : public ::testing::Test {
protected:
    static constexpr int kFrames = 60;
    static constexpr double kFps = 30.0;
    static constexpr double kDuration = kFrames / kFps;  // 2.0 seconds

    void SetUp() override {
        auto decoder = std::make_unique<MockDecoder>(kFrames, kFps);
        decoder->open("test.mp4");

        ring_ = std::make_unique<RingFrameBuffer>(16);
        decodeThread_ = std::make_unique<DecodeThread>(std::move(decoder), *ring_);
        decodeThread_->open("test.mp4");

        controller_ = std::make_unique<PlaybackController>(
            *decodeThread_, *ring_, kDuration);
    }

    void TearDown() override {
        decodeThread_->stop();
    }

    /// Let the decode thread fill some frames, then tick the controller.
    void waitForFrames(int minFrames, int maxWaitMs = 500) {
        auto start = std::chrono::steady_clock::now();
        while (ring_->size() < static_cast<size_t>(minFrames)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > maxWaitMs)
                break;
        }
    }

    std::unique_ptr<RingFrameBuffer> ring_;
    std::unique_ptr<DecodeThread> decodeThread_;
    std::unique_ptr<PlaybackController> controller_;
};

// ============================================================================
// Tests
// ============================================================================

TEST_F(PlaybackControllerTest, InitialState) {
    EXPECT_EQ(controller_->state(), PlaybackState::Stopped);
    EXPECT_DOUBLE_EQ(controller_->currentTimestamp(), 0.0);
    EXPECT_DOUBLE_EQ(controller_->playbackRate(), 1.0);
    EXPECT_DOUBLE_EQ(controller_->duration(), kDuration);
    EXPECT_DOUBLE_EQ(controller_->getProgress(), 0.0);
    EXPECT_FALSE(controller_->isFinished());
    EXPECT_EQ(controller_->underrunCount(), 0u);
}

TEST_F(PlaybackControllerTest, PlayStartsDecodeThread) {
    EXPECT_FALSE(decodeThread_->is_running());
    controller_->play();
    EXPECT_EQ(controller_->state(), PlaybackState::Playing);
    EXPECT_TRUE(decodeThread_->is_running());
}

TEST_F(PlaybackControllerTest, PauseStopsClockAdvancement) {
    controller_->play();
    waitForFrames(5);

    // Tick a few times to advance the clock
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        controller_->tick();
    }
    double posAtPause = controller_->currentTimestamp();
    EXPECT_GT(posAtPause, 0.0);

    controller_->pause();
    EXPECT_EQ(controller_->state(), PlaybackState::Paused);

    // Tick several more times — timestamp should NOT advance
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        controller_->tick();
    }
    EXPECT_DOUBLE_EQ(controller_->currentTimestamp(), posAtPause);
}

TEST_F(PlaybackControllerTest, StopResetsPosition) {
    controller_->play();
    waitForFrames(5);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    controller_->tick();
    EXPECT_GT(controller_->currentTimestamp(), 0.0);

    controller_->stop();
    EXPECT_EQ(controller_->state(), PlaybackState::Stopped);
    EXPECT_DOUBLE_EQ(controller_->currentTimestamp(), 0.0);
}

TEST_F(PlaybackControllerTest, SeekUpdatesTimestamp) {
    controller_->play();
    waitForFrames(5);

    controller_->seekTo(1.0);
    EXPECT_NEAR(controller_->currentTimestamp(), 1.0, 0.001);

    // Was playing before seek — should resume playing
    EXPECT_EQ(controller_->state(), PlaybackState::Playing);
}

TEST_F(PlaybackControllerTest, SeekWhilePausedStaysPaused) {
    controller_->play();
    waitForFrames(5);
    controller_->pause();

    controller_->seekTo(0.5);
    EXPECT_NEAR(controller_->currentTimestamp(), 0.5, 0.001);
    EXPECT_EQ(controller_->state(), PlaybackState::Paused);
}

TEST_F(PlaybackControllerTest, SeekClampsToDuration) {
    controller_->play();
    waitForFrames(1);

    controller_->seekTo(999.0);
    EXPECT_NEAR(controller_->currentTimestamp(), kDuration, 0.001);

    controller_->seekTo(-5.0);
    EXPECT_DOUBLE_EQ(controller_->currentTimestamp(), 0.0);
}

TEST_F(PlaybackControllerTest, TickPopsFrames) {
    controller_->play();
    waitForFrames(5);

    auto frame = controller_->tick();
    EXPECT_TRUE(frame.has_value());
    EXPECT_EQ(frame->width, 8u);
    EXPECT_EQ(frame->height, 8u);
}

TEST_F(PlaybackControllerTest, TickReturnsLastFrameOnUnderrun) {
    controller_->play();
    waitForFrames(3);

    // First tick gets a frame
    auto frame1 = controller_->tick();
    EXPECT_TRUE(frame1.has_value());

    // Drain the ring buffer manually
    while (ring_->try_pop().has_value()) {}

    // Next tick should return the last frame (underrun)
    bool underrun = false;
    auto frame2 = controller_->tick(&underrun);
    EXPECT_TRUE(frame2.has_value());  // still returns last frame
    // underrun may or may not be flagged depending on timing
}

TEST_F(PlaybackControllerTest, ProgressTracking) {
    controller_->play();
    waitForFrames(5);

    controller_->seekTo(kDuration / 2.0);
    EXPECT_NEAR(controller_->getProgress(), 0.5, 0.01);

    controller_->seekTo(0.0);
    EXPECT_NEAR(controller_->getProgress(), 0.0, 0.001);
}

TEST_F(PlaybackControllerTest, PlaybackRateAffectsClockSpeed) {
    controller_->setPlaybackRate(2.0);
    EXPECT_DOUBLE_EQ(controller_->playbackRate(), 2.0);

    controller_->play();
    waitForFrames(5);

    auto start = controller_->currentTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    controller_->tick();
    double elapsed = controller_->currentTimestamp() - start;

    // At 2x rate, ~50ms real time should advance ~100ms of video time.
    // Allow wide tolerance for CI/threading jitter.
    EXPECT_GT(elapsed, 0.05);
    EXPECT_LT(elapsed, 0.5);
}

TEST_F(PlaybackControllerTest, PlaybackRateClamped) {
    controller_->setPlaybackRate(0.001);
    EXPECT_DOUBLE_EQ(controller_->playbackRate(), 0.1);

    controller_->setPlaybackRate(100.0);
    EXPECT_DOUBLE_EQ(controller_->playbackRate(), 16.0);

    controller_->setPlaybackRate(1.0);
    EXPECT_DOUBLE_EQ(controller_->playbackRate(), 1.0);
}

TEST_F(PlaybackControllerTest, PlayToEnd) {
    // Use a short video — 10 frames at 30fps = 0.333s
    auto decoder = std::make_unique<MockDecoder>(10, 30.0);
    decoder->open("short.mp4");
    RingFrameBuffer ring(16);
    DecodeThread dt(std::move(decoder), ring);
    dt.open("short.mp4");
    double dur = 10.0 / 30.0;
    PlaybackController ctrl(dt, ring, dur);

    ctrl.play();

    // Tick until finished or timeout
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!ctrl.isFinished() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ctrl.tick();
    }

    EXPECT_TRUE(ctrl.isFinished());
    EXPECT_EQ(ctrl.state(), PlaybackState::Paused);  // auto-paused at end
    EXPECT_NEAR(ctrl.getProgress(), 1.0, 0.01);

    dt.stop();
}

TEST_F(PlaybackControllerTest, SetDurationUpdates) {
    EXPECT_DOUBLE_EQ(controller_->duration(), kDuration);
    controller_->setDuration(10.0);
    EXPECT_DOUBLE_EQ(controller_->duration(), 10.0);
}
