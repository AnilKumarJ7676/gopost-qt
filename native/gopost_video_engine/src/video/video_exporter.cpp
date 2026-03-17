#include "video_exporter.hpp"
#include "timeline_model.hpp"
#include "timeline_evaluator.hpp"
#include "gopost/video_engine.h"
#include <cstdio>
#include <chrono>
#include <cstring>
#include <cmath>

namespace gopost {
namespace video {

VideoExporter& VideoExporter::instance() {
    static VideoExporter inst;
    return inst;
}

int32_t VideoExporter::start(void* timeline, const ExportConfig& config) {
    auto job = std::make_unique<ExportJob>();
    int32_t id = _next_id.fetch_add(1);
    job->id = id;
    job->config = config;
    job->progress.store(0.0);
    job->cancelled.store(false);
    job->finished.store(false);

    void* tl = timeline;
    job->worker = std::thread([this, id, tl]() {
        run_export(id, tl);
    });
    job->worker.detach();

    std::lock_guard<std::mutex> lock(_mutex);
    _jobs[id] = std::move(job);
    return id;
}

double VideoExporter::get_progress(int32_t job_id) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _jobs.find(job_id);
    if (it == _jobs.end()) return -1.0;
    return it->second->progress.load();
}

int VideoExporter::cancel(int32_t job_id) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _jobs.find(job_id);
    if (it == _jobs.end()) return -1;
    it->second->cancelled.store(true);
    return 0;
}

void VideoExporter::run_export(int32_t job_id, void* timeline) {
    ExportJob* job = nullptr;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _jobs.find(job_id);
        if (it == _jobs.end()) return;
        job = it->second.get();
    }

    auto* tl = reinterpret_cast<GopostTimeline*>(timeline);
    const auto& cfg = job->config;

    double duration = 0;
    gopost_timeline_get_duration(tl, &duration);
    if (duration <= 0) {
        job->progress.store(1.0);
        job->finished.store(true);
        return;
    }

    const double frame_interval = 1.0 / cfg.frame_rate;
    const int total_frames = static_cast<int>(std::ceil(duration * cfg.frame_rate));

    // Open output file for writing encoded data
    FILE* out = std::fopen(cfg.output_path.c_str(), "wb");
    if (!out) {
        job->progress.store(-1.0);
        job->finished.store(true);
        return;
    }

    // Render each frame from the timeline evaluator
    for (int i = 0; i < total_frames; ++i) {
        if (job->cancelled.load()) {
            std::fclose(out);
            std::remove(cfg.output_path.c_str());
            job->progress.store(-1.0);
            job->finished.store(true);
            return;
        }

        double pos = i * frame_interval;
        gopost_timeline_seek(tl, pos);

        GopostFrame* frame = nullptr;
        gopost_timeline_render_frame(tl, &frame);

        if (frame && frame->data) {
            const size_t frameBytes = static_cast<size_t>(frame->stride) * frame->height;
            std::fwrite(frame->data, 1, frameBytes, out);
        }

        job->progress.store(static_cast<double>(i + 1) / total_frames);
    }

    std::fclose(out);
    job->progress.store(1.0);
    job->finished.store(true);
}

}  // namespace video
}  // namespace gopost

// --- C API wrappers ---

extern "C" {

int32_t gopost_timeline_start_export(
    GopostTimeline* timeline,
    int32_t width, int32_t height,
    double frame_rate,
    int32_t video_codec, int32_t video_bitrate_bps,
    int32_t audio_codec, int32_t audio_bitrate_kbps,
    int32_t container_format,
    const char* output_path) {

    gopost::video::ExportConfig cfg;
    cfg.width = width;
    cfg.height = height;
    cfg.frame_rate = frame_rate;
    cfg.video_codec = static_cast<gopost::video::ExportVideoCodec>(video_codec);
    cfg.video_bitrate_bps = video_bitrate_bps;
    cfg.audio_codec = static_cast<gopost::video::ExportAudioCodec>(audio_codec);
    cfg.audio_bitrate_kbps = audio_bitrate_kbps;
    cfg.container = static_cast<gopost::video::ExportContainer>(container_format);
    cfg.output_path = output_path ? output_path : "";

    return gopost::video::VideoExporter::instance().start(timeline, cfg);
}

double gopost_export_get_progress(int32_t job_id) {
    return gopost::video::VideoExporter::instance().get_progress(job_id);
}

GopostError gopost_export_cancel(int32_t job_id) {
    int result = gopost::video::VideoExporter::instance().cancel(job_id);
    return result == 0 ? GOPOST_OK : GOPOST_ERROR_INVALID_ARGUMENT;
}

}  // extern "C"
