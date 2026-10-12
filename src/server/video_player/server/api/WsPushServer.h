#pragma once

#include "video_player/server/video_store/Video.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace video_player {
namespace server {

class WsPushServer {
 public:
  WsPushServer();
  ~WsPushServer();

  // port=0 binds an ephemeral port. Clients should connect to path /ws.
  bool Start(const std::string& host, int port, std::string* error);
  void Stop();

  int bound_port() const;

  void BroadcastUpserted(const std::vector<Video>& videos);
  void BroadcastRemoved(const std::vector<std::int64_t>& ids);

 private:
  WsPushServer(const WsPushServer&);
  WsPushServer& operator=(const WsPushServer&);

  struct Impl;
  std::unique_ptr<Impl> impl_;
  std::thread thread_;
  std::atomic<int> bound_port_;
  std::atomic<bool> running_;
};

}  // namespace server
}  // namespace video_player
