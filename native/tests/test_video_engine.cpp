#include <gtest/gtest.h>
#include "gopost/video_engine.h"
#include "gopost/engine.h"

// ── Smoke tests for the video engine C API ──────────────────────────────────

TEST(VideoEngineTest, CreateAndDestroyTimeline) {
    GopostEngineConfig cfg{};
    GopostEngine* engine = nullptr;
    GopostError err = gopost_engine_create(&engine, &cfg);
    ASSERT_EQ(err, GOPOST_OK);
    ASSERT_NE(engine, nullptr);

    GopostTimelineConfig tcfg{};
    tcfg.frame_rate  = 30.0;
    tcfg.width       = 1920;
    tcfg.height      = 1080;
    tcfg.color_space = 0; // sRGB

    GopostTimeline* timeline = nullptr;
    err = gopost_timeline_create(engine, &tcfg, &timeline);
    ASSERT_EQ(err, GOPOST_OK);
    ASSERT_NE(timeline, nullptr);

    // Should have zero tracks initially
    int32_t trackCount = -1;
    err = gopost_timeline_get_track_count(timeline, &trackCount);
    EXPECT_EQ(err, GOPOST_OK);
    EXPECT_EQ(trackCount, 0);

    gopost_timeline_destroy(timeline);
    gopost_engine_destroy(engine);
}

TEST(VideoEngineTest, AddTrackAndClip) {
    GopostEngineConfig ecfg{};
    GopostEngine* engine = nullptr;
    ASSERT_EQ(gopost_engine_create(&engine, &ecfg), GOPOST_OK);

    GopostTimelineConfig tcfg{};
    tcfg.frame_rate = 24.0;
    tcfg.width      = 1280;
    tcfg.height     = 720;

    GopostTimeline* timeline = nullptr;
    ASSERT_EQ(gopost_timeline_create(engine, &tcfg, &timeline), GOPOST_OK);

    // Add a video track
    int32_t trackIdx = -1;
    ASSERT_EQ(gopost_timeline_add_track(timeline, GOPOST_TRACK_VIDEO, &trackIdx), GOPOST_OK);
    EXPECT_GE(trackIdx, 0);

    // Verify track count
    int32_t trackCount = -1;
    ASSERT_EQ(gopost_timeline_get_track_count(timeline, &trackCount), GOPOST_OK);
    EXPECT_EQ(trackCount, 1);

    // Add a color clip (no file path needed)
    GopostClipDescriptor desc{};
    desc.track_index = trackIdx;
    desc.source_type = GOPOST_CLIP_SOURCE_COLOR;
    desc.timeline_range.in_time  = 0.0;
    desc.timeline_range.out_time = 5.0;
    desc.source_range.source_in  = 0.0;
    desc.source_range.source_out = 5.0;
    desc.speed   = 1.0;
    desc.opacity = 1.0f;

    int32_t clipId = -1;
    ASSERT_EQ(gopost_timeline_add_clip(timeline, &desc, &clipId), GOPOST_OK);
    EXPECT_GE(clipId, 0);

    gopost_timeline_destroy(timeline);
    gopost_engine_destroy(engine);
}

TEST(VideoEngineTest, NullArgumentsSafety) {
    // API calls should return INVALID_ARGUMENT for null pointers
    EXPECT_EQ(gopost_engine_create(nullptr, nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(gopost_timeline_create(nullptr, nullptr, nullptr), GOPOST_ERROR_INVALID_ARGUMENT);
    // gopost_timeline_destroy(nullptr) is a void no-op — just verify it doesn't crash
    gopost_timeline_destroy(nullptr);
}
