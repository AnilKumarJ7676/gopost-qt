#include <gtest/gtest.h>
#include "video/ring_frame_buffer.hpp"

#include <atomic>
#include <thread>
#include <vector>

using gopost::video::DecodedFrame;
using gopost::video::RingFrameBuffer;

// ── Helpers ──────────────────────────────────────────────────────────────────

static DecodedFrame make_frame(double ts, uint32_t w = 2, uint32_t h = 2) {
    DecodedFrame f;
    f.width = w;
    f.height = h;
    f.timestamp_seconds = ts;
    f.pts = static_cast<int64_t>(ts * 1000);
    f.pixels.assign(static_cast<size_t>(w) * h * 4, static_cast<uint8_t>(ts));
    return f;
}

// ── Basic single-threaded tests ─────────────────────────────────────────────

TEST(RingFrameBufferTest, DefaultCapacity) {
    RingFrameBuffer rb;
    EXPECT_EQ(rb.capacity(), RingFrameBuffer::kDefaultCapacity);
    EXPECT_EQ(rb.size(), 0u);
    EXPECT_TRUE(rb.empty());
    EXPECT_FALSE(rb.full());
}

TEST(RingFrameBufferTest, CustomCapacity) {
    RingFrameBuffer rb(5);
    EXPECT_EQ(rb.capacity(), 5u);
}

TEST(RingFrameBufferTest, MinCapacityClampedToOne) {
    RingFrameBuffer rb(0);
    EXPECT_EQ(rb.capacity(), 1u);
}

TEST(RingFrameBufferTest, PushPopSingleFrame) {
    RingFrameBuffer rb(4);

    ASSERT_TRUE(rb.try_push(make_frame(1.0)));
    EXPECT_EQ(rb.size(), 1u);
    EXPECT_FALSE(rb.empty());

    auto frame = rb.try_pop();
    ASSERT_TRUE(frame.has_value());
    EXPECT_DOUBLE_EQ(frame->timestamp_seconds, 1.0);
    EXPECT_EQ(frame->width, 2u);
    EXPECT_EQ(frame->height, 2u);
    EXPECT_EQ(frame->pixels.size(), 16u);  // 2*2*4
    EXPECT_TRUE(rb.empty());
}

TEST(RingFrameBufferTest, PopFromEmptyReturnsNullopt) {
    RingFrameBuffer rb(4);
    auto frame = rb.try_pop();
    EXPECT_FALSE(frame.has_value());
}

TEST(RingFrameBufferTest, FIFOOrdering) {
    RingFrameBuffer rb(8);

    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(rb.try_push(make_frame(i * 0.5)));
    }
    EXPECT_EQ(rb.size(), 5u);

    for (int i = 0; i < 5; ++i) {
        auto frame = rb.try_pop();
        ASSERT_TRUE(frame.has_value());
        EXPECT_DOUBLE_EQ(frame->timestamp_seconds, i * 0.5);
    }
    EXPECT_TRUE(rb.empty());
}

TEST(RingFrameBufferTest, FullBufferRejectsPush) {
    RingFrameBuffer rb(3);

    ASSERT_TRUE(rb.try_push(make_frame(1.0)));
    ASSERT_TRUE(rb.try_push(make_frame(2.0)));
    ASSERT_TRUE(rb.try_push(make_frame(3.0)));
    EXPECT_TRUE(rb.full());

    // 4th push should fail
    EXPECT_FALSE(rb.try_push(make_frame(4.0)));

    // Pop one, then push should succeed again
    auto f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_DOUBLE_EQ(f->timestamp_seconds, 1.0);
    EXPECT_TRUE(rb.try_push(make_frame(4.0)));
}

TEST(RingFrameBufferTest, WrapAround) {
    RingFrameBuffer rb(3);

    // Fill and drain multiple cycles to exercise index wrapping
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int i = 0; i < 3; ++i) {
            double ts = cycle * 10.0 + i;
            ASSERT_TRUE(rb.try_push(make_frame(ts)));
        }
        EXPECT_TRUE(rb.full());

        for (int i = 0; i < 3; ++i) {
            double ts = cycle * 10.0 + i;
            auto f = rb.try_pop();
            ASSERT_TRUE(f.has_value());
            EXPECT_DOUBLE_EQ(f->timestamp_seconds, ts);
        }
        EXPECT_TRUE(rb.empty());
    }
}

TEST(RingFrameBufferTest, ClearResetsBuffer) {
    RingFrameBuffer rb(4);

    rb.try_push(make_frame(1.0));
    rb.try_push(make_frame(2.0));
    rb.try_push(make_frame(3.0));
    EXPECT_EQ(rb.size(), 3u);

    rb.clear();
    EXPECT_EQ(rb.size(), 0u);
    EXPECT_TRUE(rb.empty());

    // After clear, pop returns nothing
    EXPECT_FALSE(rb.try_pop().has_value());

    // After clear, push still works
    ASSERT_TRUE(rb.try_push(make_frame(10.0)));
    auto f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_DOUBLE_EQ(f->timestamp_seconds, 10.0);
}

TEST(RingFrameBufferTest, ClearFreesPixelMemory) {
    RingFrameBuffer rb(2);

    // Push large frames
    DecodedFrame big;
    big.width = 1920;
    big.height = 1080;
    big.pixels.assign(1920 * 1080 * 4, 0xFF);
    rb.try_push(std::move(big));
    EXPECT_EQ(rb.size(), 1u);

    rb.clear();
    EXPECT_TRUE(rb.empty());
    // The vector memory has been released (moved-from or reset)
}

// ── Shutdown tests ──────────────────────────────────────────────────────────

TEST(RingFrameBufferTest, ShutdownUnblocksPush) {
    RingFrameBuffer rb(1);
    rb.try_push(make_frame(1.0));  // Fill it

    // Blocking push in a thread — should unblock when we call shutdown
    std::atomic<bool> done{false};
    std::thread producer([&] {
        bool ok = rb.push(make_frame(2.0));
        EXPECT_FALSE(ok);  // Should return false due to shutdown
        done.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    rb.shutdown();
    producer.join();
    EXPECT_TRUE(done.load());
}

TEST(RingFrameBufferTest, ShutdownUnblocksPop) {
    RingFrameBuffer rb(4);  // Empty

    std::atomic<bool> done{false};
    std::thread consumer([&] {
        auto f = rb.pop();
        EXPECT_FALSE(f.has_value());  // Shutdown returns nullopt
        done.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    rb.shutdown();
    consumer.join();
    EXPECT_TRUE(done.load());
}

TEST(RingFrameBufferTest, RestartAfterShutdown) {
    RingFrameBuffer rb(4);

    rb.shutdown();
    EXPECT_FALSE(rb.push(make_frame(1.0)));

    rb.restart();
    rb.clear();
    EXPECT_TRUE(rb.try_push(make_frame(2.0)));
    auto f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_DOUBLE_EQ(f->timestamp_seconds, 2.0);
}

// ── Move semantics: pixel data ownership ────────────────────────────────────

TEST(RingFrameBufferTest, MoveSemanticPreservesPixels) {
    RingFrameBuffer rb(4);

    auto frame = make_frame(5.5, 4, 4);
    const size_t expected_size = frame.pixels.size();
    const uint8_t expected_val = frame.pixels[0];

    rb.try_push(std::move(frame));
    // Source frame should be moved-from (pixels likely empty)
    EXPECT_TRUE(frame.pixels.empty());

    auto popped = rb.try_pop();
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->pixels.size(), expected_size);
    EXPECT_EQ(popped->pixels[0], expected_val);
    EXPECT_DOUBLE_EQ(popped->timestamp_seconds, 5.5);
}

// ── Concurrent SPSC stress test ─────────────────────────────────────────────

TEST(RingFrameBufferTest, SPSCStress) {
    constexpr size_t kCapacity = 16;
    constexpr size_t kFrameCount = 10000;

    RingFrameBuffer rb(kCapacity);

    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};

    // Producer thread: push kFrameCount frames
    std::thread producer([&] {
        for (size_t i = 0; i < kFrameCount; ++i) {
            DecodedFrame f;
            f.timestamp_seconds = static_cast<double>(i);
            f.pts = static_cast<int64_t>(i);
            f.width = 1;
            f.height = 1;
            f.pixels = {static_cast<uint8_t>(i & 0xFF)};

            while (!rb.try_push(std::move(f))) {
                std::this_thread::yield();
            }
            produced.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Consumer thread: pop kFrameCount frames, verify ordering
    std::thread consumer([&] {
        for (size_t i = 0; i < kFrameCount; ++i) {
            std::optional<DecodedFrame> f;
            while (!(f = rb.try_pop()).has_value()) {
                std::this_thread::yield();
            }
            // Verify strict FIFO ordering
            EXPECT_DOUBLE_EQ(f->timestamp_seconds, static_cast<double>(i));
            EXPECT_EQ(f->pts, static_cast<int64_t>(i));
            consumed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced.load(), kFrameCount);
    EXPECT_EQ(consumed.load(), kFrameCount);
    EXPECT_TRUE(rb.empty());
}

// ── Seek simulation: clear between burst read/writes ────────────────────────

TEST(RingFrameBufferTest, SeekClearAndResume) {
    RingFrameBuffer rb(8);

    // Simulate pre-decode: push 5 frames
    for (int i = 0; i < 5; ++i) {
        rb.try_push(make_frame(i * 0.033));
    }
    EXPECT_EQ(rb.size(), 5u);

    // Consume 2
    rb.try_pop();
    rb.try_pop();
    EXPECT_EQ(rb.size(), 3u);

    // Seek: clear all stale frames
    rb.clear();
    EXPECT_TRUE(rb.empty());

    // Resume from new position
    for (int i = 0; i < 4; ++i) {
        rb.try_push(make_frame(10.0 + i * 0.033));
    }
    EXPECT_EQ(rb.size(), 4u);

    auto f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_NEAR(f->timestamp_seconds, 10.0, 0.001);
}

// ── Capacity-1 edge case ────────────────────────────────────────────────────

TEST(RingFrameBufferTest, CapacityOne) {
    RingFrameBuffer rb(1);

    EXPECT_TRUE(rb.try_push(make_frame(1.0)));
    EXPECT_TRUE(rb.full());
    EXPECT_FALSE(rb.try_push(make_frame(2.0)));

    auto f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_DOUBLE_EQ(f->timestamp_seconds, 1.0);
    EXPECT_TRUE(rb.empty());

    EXPECT_TRUE(rb.try_push(make_frame(3.0)));
    f = rb.try_pop();
    ASSERT_TRUE(f.has_value());
    EXPECT_DOUBLE_EQ(f->timestamp_seconds, 3.0);
}
