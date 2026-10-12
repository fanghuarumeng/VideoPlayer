#pragma once

#include "video_player/server/ingest/IngestConfig.h"
#include "video_player/server/video_store/IVideoRepository.h"
#include "video_player/server/video_store/Video.h"
#include "video_player/server/video_source/IVideoSource.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace video_player {
namespace server {

class IngestService {
 public:
  typedef std::function<void(const std::vector<Video>&)> UpsertedHandler;

  IngestService(const std::vector<IVideoSource*>& sources,
                IVideoRepository* repo,
                const IngestConfig& config);
  ~IngestService();

  void set_upserted_handler(UpsertedHandler handler);
  void start();
  void stop();
  void runOnce();

 private:
  IngestService(const IngestService&);
  IngestService& operator=(const IngestService&);

  void Loop();
  static std::string NowUtcIso();
  static Video CandidateToVideo(const VideoCandidate& candidate,
                                const std::string& now_utc,
                                const Video* existing);

  std::vector<IVideoSource*> sources_;
  IVideoRepository* repo_;
  IngestConfig config_;
  UpsertedHandler upserted_handler_;
  std::mutex handler_mutex_;

  std::atomic<bool> stop_;
  std::atomic<bool> running_;
  std::mutex wait_mutex_;
  std::condition_variable wait_cv_;
  std::thread worker_;
};

}  // namespace server
}  // namespace video_player
