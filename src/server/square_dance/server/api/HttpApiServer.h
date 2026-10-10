#pragma once

#include "square_dance/server/ingest/IngestService.h"
#include "square_dance/server/local_media/LocalMediaStore.h"
#include "square_dance/server/local_media/UploadGate.h"
#include "square_dance/server/video_store/IVideoRepository.h"
#include "square_dance/server/video_store/Video.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace square_dance {
namespace server {

class HttpApiServer {
 public:
  typedef std::function<void(const Video&)> UpsertedHandler;

  HttpApiServer(IVideoRepository* repo, IngestService* ingest);
  ~HttpApiServer();

  void set_local_media(LocalMediaStore* store, UploadGate* gate);
  void set_upserted_handler(UpsertedHandler handler);

  // Bind and listen on a background thread. port=0 binds an ephemeral port.
  bool Start(const std::string& host, int port, std::string* error);
  void Stop();

  int bound_port() const;

 private:
  HttpApiServer(const HttpApiServer&);
  HttpApiServer& operator=(const HttpApiServer&);

  void RegisterRoutes();
  void ServeLoop();
  void RefreshHttpBase();
  void NotifyUpserted(const Video& video);

  IVideoRepository* repo_;
  IngestService* ingest_;
  LocalMediaStore* media_store_;
  UploadGate* upload_gate_;
  UpsertedHandler upserted_handler_;
  std::mutex handler_mutex_;

  std::string host_;
  int requested_port_;
  std::atomic<int> bound_port_;
  std::atomic<bool> stop_;
  std::thread thread_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace server
}  // namespace square_dance
