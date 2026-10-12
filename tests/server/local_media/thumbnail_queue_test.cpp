#include "video_player/server/local_media/ThumbnailGenerator.h"
#include "video_player/server/local_media/ThumbnailQueue.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace {

class CountingGenerator : public video_player::server::IThumbnailGenerator {
 public:
  CountingGenerator(int sleep_ms, std::atomic<int>* peak,
                    std::atomic<int>* current, std::atomic<int>* done)
      : sleep_ms_(sleep_ms), peak_(peak), current_(current), done_(done) {}

  virtual bool generate(const video_player::server::ThumbJob&,
                        std::string*) {
    const int now = ++(*current_);
    int peak = peak_->load();
    while (now > peak && !peak_->compare_exchange_weak(peak, now)) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms_));
    --(*current_);
    ++(*done_);
    return true;
  }

 private:
  int sleep_ms_;
  std::atomic<int>* peak_;
  std::atomic<int>* current_;
  std::atomic<int>* done_;
};

}  // namespace

TEST(ThumbnailQueue, CapsConcurrentWorkers) {
  std::atomic<int> peak(0);
  std::atomic<int> current(0);
  std::atomic<int> done(0);
  CountingGenerator gen(80, &peak, &current, &done);
  video_player::server::ThumbnailQueue queue(&gen);
  queue.start(1);

  const int n = 5;
  for (int i = 0; i < n; ++i) {
    video_player::server::ThumbJob job;
    job.video_id = i + 1;
    job.video_path = "in.mp4";
    job.thumb_path = "out.jpg";
    job.thumb_filename = "out.jpg";
    queue.enqueue(job);
  }

  for (int i = 0; i < 100 && done.load() < n; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  queue.stop();

  EXPECT_EQ(done.load(), n);
  EXPECT_LE(peak.load(), 1);
}

TEST(ThumbnailQueue, AllowsConfiguredWorkers) {
  std::atomic<int> peak(0);
  std::atomic<int> current(0);
  std::atomic<int> done(0);
  CountingGenerator gen(100, &peak, &current, &done);
  video_player::server::ThumbnailQueue queue(&gen);
  queue.start(2);

  for (int i = 0; i < 4; ++i) {
    video_player::server::ThumbJob job;
    job.video_id = i + 1;
    job.video_path = "in.mp4";
    job.thumb_path = "out.jpg";
    job.thumb_filename = "out.jpg";
    queue.enqueue(job);
  }

  for (int i = 0; i < 100 && done.load() < 4; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  queue.stop();

  EXPECT_EQ(done.load(), 4);
  EXPECT_LE(peak.load(), 2);
  EXPECT_GE(peak.load(), 2);
}
