#pragma once

#include "square_dance/server/local_media/ThumbnailGenerator.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace square_dance {
namespace server {

class ThumbnailQueue {
 public:
  typedef std::function<void(const ThumbJob& job, bool ok)> CompletedHandler;

  explicit ThumbnailQueue(IThumbnailGenerator* generator);
  ~ThumbnailQueue();

  void set_completed_handler(CompletedHandler handler);

  void start(int worker_count);
  void stop();
  void enqueue(const ThumbJob& job);

  std::size_t pending_count() const;

 private:
  ThumbnailQueue(const ThumbnailQueue&);
  ThumbnailQueue& operator=(const ThumbnailQueue&);

  void WorkerLoop();

  IThumbnailGenerator* generator_;
  CompletedHandler completed_handler_;
  mutable std::mutex handler_mutex_;

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<ThumbJob> jobs_;
  std::vector<std::thread> workers_;
  std::atomic<bool> stop_;
  std::atomic<bool> running_;
};

}  // namespace server
}  // namespace square_dance
