#ifndef GOPOST_VIDEO_EXPORTER_HPP
#define GOPOST_VIDEO_EXPORTER_HPP

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>

namespace gopost {
namespace video {

enum class ExportVideoCodec { H264 = 0, H265 = 1, VP9 = 2 };
enum class ExportAudioCodec { AAC = 0, Opus = 1 };
enum class ExportContainer { MP4 = 0, MOV = 1, WebM = 2 };

struct ExportConfig {
    int32_t width = 1920;
    int32_t height = 1080;
    double frame_rate = 30.0;
    ExportVideoCodec video_codec = ExportVideoCodec::H264;
    int32_t video_bitrate_bps = 8000000;
    ExportAudioCodec audio_codec = ExportAudioCodec::AAC;
    int32_t audio_bitrate_kbps = 192;
    ExportContainer container = ExportContainer::MP4;
    std::string output_path;
};

struct ExportJob {
    int32_t id;
    ExportConfig config;
    std::atomic<double> progress{0.0};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> finished{false};
    std::thread worker;
};

/// Manages asynchronous video export jobs.
/// Each job renders the timeline frame-by-frame and encodes to the output file.
class VideoExporter {
public:
    static VideoExporter& instance();

    int32_t start(void* timeline, const ExportConfig& config);
    double  get_progress(int32_t job_id);
    int     cancel(int32_t job_id);

private:
    VideoExporter() = default;
    void run_export(int32_t job_id, void* timeline);

    std::mutex _mutex;
    std::unordered_map<int32_t, std::unique_ptr<ExportJob>> _jobs;
    std::atomic<int32_t> _next_id{1};
};

}  // namespace video
}  // namespace gopost

#endif
