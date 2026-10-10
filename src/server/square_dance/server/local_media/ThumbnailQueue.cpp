#include "square_dance/server/local_media/ThumbnailQueue.h"

#include "square_dance/server/log.h"

namespace square_dance {
namespace server {

ThumbnailQueue::ThumbnailQueue(IThumbnailGenerator* generator)
    : generator_(generator), stop_(true), running_(false) {}

ThumbnailQueue::~ThumbnailQueue() {
  stop();
}

void ThumbnailQueue::set_completed_handler(CompletedHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  completed_handler_ = handler;
}

void ThumbnailQueue::start(int worker_count) {
  if (running_) {
    return;
  }
  if (worker_count < 1) {
    worker_count = 1;
  }
  if (worker_count > 4) {
    worker_count = 4;
  }
  stop_ = false;
  running_ = true;
  workers_.clear();
  workers_.reserve(static_cast<std::size_t>(worker_count));
  for (int i = 0; i < worker_count; ++i) {
    workers_.push_back(std::thread(&ThumbnailQueue::WorkerLoop, this));
  }
}

void ThumbnailQueue::stop() {
  if (!running_) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cv_.notify_all();
  for (std::size_t i = 0; i < workers_.size(); ++i) {
    if (workers_[i].joinable()) {
      workers_[i].join();
    }
  }
  workers_.clear();
  running_ = false;
  std::lock_guard<std::mutex> lock(mutex_);
  while (!jobs_.empty()) {
    jobs_.pop();
  }
}

void ThumbnailQueue::enqueue(const ThumbJob& job) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      return;
    }
    jobs_.push(job);
  }
  cv_.notify_one();
}

std::size_t ThumbnailQueue::pending_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return jobs_.size();
}

void ThumbnailQueue::WorkerLoop() {
  while (true) {
    ThumbJob job;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]() { return stop_.load() || !jobs_.empty(); });
      if (stop_ && jobs_.empty()) {
        return;
      }
      if (jobs_.empty()) {
        continue;
      }
      job = jobs_.front();
      jobs_.pop();
    }

    bool ok = false;
    std::string error;
    if (generator_ != NULL) {
      ok = generator_->generate(job, &error);
      if (!ok) {
        LogWarn("thumbnail job failed id=" +
                std::to_string(static_cast<long long>(job.video_id)) + ": " +
                error);
      }
    }

    CompletedHandler handler;
    {
      std::lock_guard<std::mutex> lock(handler_mutex_);
      handler = completed_handler_;
    }
    if (handler) {
      handler(job, ok);
    }
  }
}

}  // namespace server
}  // namespace square_dance
