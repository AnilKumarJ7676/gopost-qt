#ifndef GOPOST_VIDEO_THUMBNAIL_GENERATOR_HPP
#define GOPOST_VIDEO_THUMBNAIL_GENERATOR_HPP

#include "decoder_pool.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct GopostEngine;

namespace gopost {
namespace video {

/// Result of a single thumbnail extraction.
struct ThumbnailFrame {
    std::vector<uint8_t> jpeg_data;  // JPEG-compressed thumbnail
    int32_t width = 0;
    int32_t height = 0;
    double timestamp = 0;
};

/// Status of a thumbnail job.
enum class ThumbnailJobStatus {
    Queued,
    InProgress,
    Completed,
    Failed,
    Cancelled,
};

/// A request to extract thumbnails from a single video file.
struct ThumbnailRequest {
    int32_t job_id = 0;
    std::string source_path;
    double source_duration = 0;
    int32_t count = 0;           // Number of thumbnails to extract
    int32_t thumb_width = 160;   // Output thumbnail width
    int32_t thumb_height = 90;   // Output thumbnail height
    DecoderPriority priority = DecoderPriority::Medium;

    /// Callback invoked on the worker thread when each frame is ready.
    /// Can be used for incremental UI updates.
    std::function<void(int32_t frame_index, const ThumbnailFrame& frame)> on_frame;
    /// Callback invoked when the entire job completes or fails.
    std::function<void(int32_t job_id, ThumbnailJobStatus status)> on_complete;
};

/// Sequential thumbnail generator using the DecoderPool.
///
/// Processes one video at a time: opens decoder, extracts all frames, closes
/// decoder, then moves to the next video. This prevents concurrent decoder
/// resource exhaustion that causes videos 2 and 4 to fail when loading 5 videos.
///
/// Jobs are processed in priority order (High > Medium > Low).
class ThumbnailGenerator {
public:
    /// @param pool   Shared decoder pool (not owned).
    /// @param engine Engine pointer for frame scaling.
    ThumbnailGenerator(DecoderPool* pool, GopostEngine* engine);
    ~ThumbnailGenerator();

    ThumbnailGenerator(const ThumbnailGenerator&) = delete;
    ThumbnailGenerator& operator=(const ThumbnailGenerator&) = delete;

    /// Submit a thumbnail extraction request. Returns the job_id.
    /// The job is processed asynchronously on a background thread.
    int32_t submit(ThumbnailRequest request);

    /// Cancel a pending or in-progress job.
    void cancel(int32_t job_id);

    /// Cancel all pending jobs.
    void cancel_all();

    /// Query job status.
    ThumbnailJobStatus job_status(int32_t job_id) const;

    /// Get completed thumbnails for a job. Returns empty if not done.
    std::vector<ThumbnailFrame> get_results(int32_t job_id) const;

    /// Number of jobs in the queue (including the one currently processing).
    int32_t queue_size() const;

    /// Shut down the worker thread.
    void shutdown();

private:
    struct Job {
        ThumbnailRequest request;
        ThumbnailJobStatus status = ThumbnailJobStatus::Queued;
        std::vector<ThumbnailFrame> results;
    };

    void worker_loop();
    void process_job(Job& job);
    bool encode_rgba_to_jpeg(const uint8_t* rgba, int w, int h,
                             int out_w, int out_h,
                             std::vector<uint8_t>& out_jpeg);
    void scale_rgba(const uint8_t* src, int src_w, int src_h,
                    uint8_t* dst, int dst_w, int dst_h);

    DecoderPool* pool_;
    GopostEngine* engine_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<int32_t> queue_;  // job IDs in priority order
    std::unordered_map<int32_t, std::unique_ptr<Job>> jobs_;
    std::atomic<int32_t> next_job_id_{1};

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<int32_t> current_job_id_{-1};
};

}  // namespace video
}  // namespace gopost

#endif  // GOPOST_VIDEO_THUMBNAIL_GENERATOR_HPP
