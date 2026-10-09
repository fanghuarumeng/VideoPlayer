#pragma once

#include "square_dance/server/ingest/IngestService.h"
#include "square_dance/server/video_store/IVideoRepository.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace square_dance {
namespace server {

class HttpApiServer {
 public:
  HttpApiServer(IVideoRepository* repo, IngestService* ingest);
  ~HttpApiServer();

  // Bind and listen on a background thread. port=0 binds an ephemeral port.
  bool Start(const std::string& host, int port, std::string* error);
  void Stop();

  int bound_port() const;

 private:
  HttpApiServer(const HttpApiServer&);
  HttpApiServer& operator=(const HttpApiServer&);

  void RegisterRoutes();
  void ServeLoop();

  IVideoRepository* repo_;
  IngestService* ingest_;
  std::string host_;
  int requested_port_;
  std::atomic<int> bound_port_;
  std::atomic<bool> stop_;
  std::thread thread_;

  // Opaque httplib::Server* to keep httplib.h out of this header.
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace server
}  // namespace square_dance
