#pragma once

#include "square_dance/server/ServerConfig.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace square_dance {
namespace server {

class IVideoSource;
class SqliteVideoRepository;
class IngestService;
class UrlProber;
class UrlHealthService;
class HttpApiServer;
class WsPushServer;
class LocalMediaStore;
class UploadGate;

class ServerApp {
 public:
  explicit ServerApp(const ServerConfig& config);
  ~ServerApp();

  const ServerConfig& config() const;

  bool Start(std::string* error);
  void RequestStop();
  void Stop();

  // Starts services and blocks until RequestStop (Ctrl+C / console close).
  int RunBlocking();

  int http_port() const;
  int ws_port() const;

 private:
  ServerApp(const ServerApp&);
  ServerApp& operator=(const ServerApp&);

  bool EnsureSqliteParentDir(std::string* error);
  void WireHandlers();

  ServerConfig config_;
  std::atomic<bool> started_;
  std::atomic<bool> stop_requested_;
  std::mutex stop_mutex_;
  std::condition_variable stop_cv_;

  std::unique_ptr<SqliteVideoRepository> repo_;
  std::unique_ptr<IVideoSource> authorized_source_;
  std::unique_ptr<IVideoSource> archive_source_;
  std::vector<IVideoSource*> source_ptrs_;
  std::unique_ptr<IngestService> ingest_;
  std::unique_ptr<UrlProber> prober_;
  std::unique_ptr<UrlHealthService> health_;
  std::unique_ptr<LocalMediaStore> media_store_;
  std::unique_ptr<UploadGate> upload_gate_;
  std::unique_ptr<HttpApiServer> http_;
  std::unique_ptr<WsPushServer> ws_;
};

}  // namespace server
}  // namespace square_dance
