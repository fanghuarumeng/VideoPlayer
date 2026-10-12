#include "video_player/server/ingest/IngestService.h"

#include "video_player/server/log.h"

#include <chrono>
#include <ctime>
#include <sstream>

namespace video_player {
namespace server {
namespace {

bool SameContent(const Video& existing, const VideoCandidate& candidate) {
  return existing.title == candidate.title &&
         existing.play_url == candidate.play_url &&
         existing.download_url == candidate.download_url &&
         existing.source_type == candidate.source_type;
}

}  // namespace

IngestService::IngestService(const std::vector<IVideoSource*>& sources,
                             IVideoRepository* repo,
                             const IngestConfig& config)
    : sources_(sources),
      repo_(repo),
      config_(config),
      stop_(true),
      running_(false) {}

IngestService::~IngestService() {
  stop();
}

void IngestService::set_upserted_handler(UpsertedHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  upserted_handler_ = handler;
}

void IngestService::start() {
  if (running_) {
    return;
  }
  stop_ = false;
  running_ = true;
  worker_ = std::thread(&IngestService::Loop, this);
}

void IngestService::stop() {
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

void IngestService::Loop() {
  while (!stop_) {
    runOnce();
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wait_cv_.wait_for(
        lock, std::chrono::milliseconds(config_.interval_ms),
        [this]() { return stop_.load(); });
  }
}

std::string IngestService::NowUtcIso() {
  const std::time_t now = std::time(NULL);
  std::tm tm_buf;
#if defined(_WIN32)
  gmtime_s(&tm_buf, &now);
#else
  gmtime_r(&now, &tm_buf);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
  return std::string(buf);
}

Video IngestService::CandidateToVideo(const VideoCandidate& candidate,
                                      const std::string& now_utc,
                                      const Video* existing) {
  Video video;
  video.id = existing != NULL ? existing->id : 0;
  video.external_id = candidate.external_id;
  video.title = candidate.title;
  video.play_url = candidate.play_url;
  video.download_url = candidate.download_url;
  video.source_type = candidate.source_type;
  video.created_at = existing != NULL ? existing->created_at : now_utc;
  video.updated_at = now_utc;
  if (existing == NULL) {
    video.consecutive_failures = 0;
  } else if (existing->play_url != candidate.play_url ||
             existing->download_url != candidate.download_url) {
    video.consecutive_failures = 0;
  } else {
    video.consecutive_failures = existing->consecutive_failures;
  }
  return video;
}

void IngestService::runOnce() {
  if (repo_ == NULL) {
    LogError("IngestService::runOnce repository is null");
    return;
  }

  const std::string now = NowUtcIso();
  std::vector<Video> changed;
  int failed_sources = 0;

  for (std::size_t si = 0; si < sources_.size(); ++si) {
    IVideoSource* source = sources_[si];
    if (source == NULL) {
      continue;
    }

    for (std::size_t ki = 0; ki < config_.keywords.size(); ++ki) {
      std::string error;
      const std::vector<VideoCandidate> candidates = source->search(
          config_.keywords[ki], config_.per_keyword_limit, &error);
      if (!error.empty() && candidates.empty()) {
        LogWarn("ingest source " + source->id() + " failed: " + error);
        ++failed_sources;
        continue;
      }

      for (std::size_t ci = 0; ci < candidates.size(); ++ci) {
        const VideoCandidate& candidate = candidates[ci];
        Video existing;
        std::string find_error;
        const bool found = repo_->findBySourceKey(
            candidate.source_type, candidate.external_id, &existing,
            &find_error);

        if (found && SameContent(existing, candidate)) {
          continue;
        }

        const Video video =
            CandidateToVideo(candidate, now, found ? &existing : NULL);
        std::string upsert_error;
        if (!repo_->upsert(video, &upsert_error)) {
          LogWarn("ingest upsert failed: " + upsert_error);
          continue;
        }

        Video stored;
        if (repo_->findBySourceKey(candidate.source_type, candidate.external_id,
                                   &stored, &find_error)) {
          changed.push_back(stored);
        } else {
          changed.push_back(video);
        }
      }
    }
  }

  if (!changed.empty()) {
    UpsertedHandler handler;
    {
      std::lock_guard<std::mutex> lock(handler_mutex_);
      handler = upserted_handler_;
    }
    if (handler) {
      handler(changed);
    }
  }

  std::ostringstream ss;
  ss << "ingest runOnce finished; changed=" << changed.size()
     << " source_failures=" << failed_sources;
  LogInfo(ss.str());
}

}  // namespace server
}  // namespace video_player
