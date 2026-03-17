#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace gopost {

class ThreadPool {
public:
    enum class Priority : int {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3,
    };

    explicit ThreadPool(size_t thread_count = 0)
        : running_(true) {
        if (thread_count == 0) {
            thread_count = std::max(1u, std::thread::hardware_concurrency() - 1);
        }

        workers_.reserve(thread_count);
        for (size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task, Priority priority = Priority::Normal) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push({std::move(task), static_cast<int>(priority)});
        }
        cv_.notify_one();
    }

    size_t thread_count() const { return workers_.size(); }
    size_t pending_tasks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    struct Task {
        std::function<void()> func;
        int priority;

        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    void worker_loop() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return !running_ || !tasks_.empty(); });
                if (!running_ && tasks_.empty()) return;
                task = tasks_.top();
                tasks_.pop();
            }
            task.func();
        }
    }

    std::vector<std::thread> workers_;
    std::priority_queue<Task> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
};

}  // namespace gopost
