#pragma once

#include "video_player/server/ingest/IngestService.h"
#include "video_player/server/local_media/LocalMediaStore.h"
#include "video_player/server/local_media/ThumbnailQueue.h"
#include "video_player/server/local_media/UploadGate.h"
#include "video_player/server/video_store/IVideoRepository.h"
#include "video_player/server/video_store/Video.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace video_player {
namespace server {

class HttpApiServer {
 public:
  typedef std::function<void(const Video&)> UpsertedHandler;

  HttpApiServer(IVideoRepository* repo, IngestService* ingest);
  ~HttpApiServer();

  void set_local_media(LocalMediaStore* store, UploadGate* gate);
  void set_thumbnail_queue(ThumbnailQueue* queue);
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
  ThumbnailQueue* thumb_queue_;
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
}  // namespace video_player
