#pragma once

#include "video_player/server/url_health/IUrlProber.h"
#include "video_player/server/video_store/IVideoRepository.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace video_player {
namespace server {

struct UrlHealthConfig {
  int interval_ms;
  int batch_limit;

  static UrlHealthConfig MakeDefault() {
    UrlHealthConfig cfg;
    cfg.interval_ms = 12 * 60 * 60 * 1000;
    cfg.batch_limit = 1000;
    return cfg;
  }
};

class UrlHealthService {
 public:
  static const int kFailureThreshold = 3;
  typedef std::function<void(const std::vector<std::int64_t>&)> RemovedHandler;

  UrlHealthService(IVideoRepository* repo,
                   IUrlProber* prober,
                   const UrlHealthConfig& config);
  ~UrlHealthService();

  void set_removed_handler(RemovedHandler handler);
  void start();
  void stop();
  void runOnce();

 private:
  UrlHealthService(const UrlHealthService&);
  UrlHealthService& operator=(const UrlHealthService&);

  void Loop();
  bool ProbeVideo(const Video& video, std::string* error);

  IVideoRepository* repo_;
  IUrlProber* prober_;
  UrlHealthConfig config_;
  RemovedHandler removed_handler_;
  std::mutex handler_mutex_;

  std::atomic<bool> stop_;
  std::atomic<bool> running_;
  std::mutex wait_mutex_;
  std::condition_variable wait_cv_;
  std::thread worker_;
};

}  // namespace server
}  // namespace video_player
