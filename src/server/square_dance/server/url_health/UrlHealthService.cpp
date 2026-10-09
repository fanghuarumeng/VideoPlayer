#include "square_dance/server/url_health/UrlHealthService.h"

#include "square_dance/server/log.h"

#include <chrono>
#include <sstream>

namespace square_dance {
namespace server {

UrlHealthService::UrlHealthService(IVideoRepository* repo,
                                   IUrlProber* prober,
                                   const UrlHealthConfig& config)
    : repo_(repo),
      prober_(prober),
      config_(config),
      stop_(true),
      running_(false) {}

UrlHealthService::~UrlHealthService() {
  stop();
}

void UrlHealthService::set_removed_handler(RemovedHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  removed_handler_ = handler;
}

void UrlHealthService::start() {
  if (running_) {
    return;
  }
  stop_ = false;
  running_ = true;
  worker_ = std::thread(&UrlHealthService::Loop, this);
}

void UrlHealthService::stop() {
  if (!running_) {
    return;
  }
  stop_ = true;
  wait_cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
  running_ = false;
}

void UrlHealthService::Loop() {
  while (!stop_) {
    runOnce();
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wait_cv_.wait_for(
        lock, std::chrono::milliseconds(config_.interval_ms),
        [this]() { return stop_.load(); });
  }
}

bool UrlHealthService::ProbeVideo(const Video& video, std::string* error) {
  if (prober_ == NULL) {
    if (error != NULL) {
      *error = "prober is null";
    }
    return false;
  }
  if (!prober_->isReachable(video.play_url, error)) {
    return false;
  }
  if (video.download_url != video.play_url) {
    if (!prober_->isReachable(video.download_url, error)) {
      return false;
    }
  }
  return true;
}

void UrlHealthService::runOnce() {
  if (repo_ == NULL || prober_ == NULL) {
    LogError("UrlHealthService::runOnce missing repo/prober");
    return;
  }

  const std::vector<Video> videos =
      repo_->listAll(config_.batch_limit > 0 ? config_.batch_limit : 1000, 0);
  std::vector<std::int64_t> removed_ids;

  for (std::size_t i = 0; i < videos.size(); ++i) {
    Video video = videos[i];
    std::string probe_error;
    const bool ok = ProbeVideo(video, &probe_error);
    if (ok) {
      if (video.consecutive_failures != 0) {
        video.consecutive_failures = 0;
        std::string upsert_error;
        if (!repo_->upsert(video, &upsert_error)) {
          LogWarn("url-health reset failures failed: " + upsert_error);
        }
      }
      continue;
    }

    video.consecutive_failures += 1;
    if (video.consecutive_failures >= kFailureThreshold) {
      std::string remove_error;
      if (repo_->removeById(video.id, &remove_error)) {
        removed_ids.push_back(video.id);
        LogInfo("url-health removed unreachable video id=" +
                std::to_string(static_cast<long long>(video.id)));
      } else {
        LogWarn("url-health remove failed: " + remove_error);
      }
    } else {
      std::string upsert_error;
      if (!repo_->upsert(video, &upsert_error)) {
        LogWarn("url-health increment failures failed: " + upsert_error);
      }
    }
  }

  if (!removed_ids.empty()) {
    RemovedHandler handler;
    {
      std::lock_guard<std::mutex> lock(handler_mutex_);
      handler = removed_handler_;
    }
    if (handler) {
      handler(removed_ids);
    }
  }

  std::ostringstream ss;
  ss << "url-health runOnce finished; scanned=" << videos.size()
     << " removed=" << removed_ids.size();
  LogInfo(ss.str());
}

}  // namespace server
}  // namespace square_dance
