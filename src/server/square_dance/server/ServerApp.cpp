#include "square_dance/server/ServerApp.h"

#include "square_dance/server/api/HttpApiServer.h"
#include "square_dance/server/api/WsPushServer.h"
#include "square_dance/server/ingest/IngestConfig.h"
#include "square_dance/server/ingest/IngestService.h"
#include "square_dance/server/log.h"
#include "square_dance/server/url_health/UrlHealthService.h"
#include "square_dance/server/url_health/UrlProber.h"
#include "square_dance/server/video_source/ArchiveOrgSource.h"
#include "square_dance/server/video_source/AuthorizedCatalogSource.h"
#include "square_dance/server/video_store/SqliteVideoRepository.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <sstream>

namespace square_dance {
namespace server {
namespace {

#if defined(_WIN32)
ServerApp* g_console_app = NULL;

BOOL WINAPI ConsoleCtrlHandler(DWORD) {
  if (g_console_app != NULL) {
    g_console_app->RequestStop();
  }
  return TRUE;
}
#endif

}  // namespace

ServerApp::ServerApp(const ServerConfig& config)
    : config_(config), started_(false), stop_requested_(false) {}

ServerApp::~ServerApp() {
  Stop();
}

const ServerConfig& ServerApp::config() const {
  return config_;
}

int ServerApp::http_port() const {
  return http_.get() != NULL ? http_->bound_port() : 0;
}

int ServerApp::ws_port() const {
  return ws_.get() != NULL ? ws_->bound_port() : 0;
}

bool ServerApp::EnsureSqliteParentDir(std::string* error) {
  const std::string& path = config_.sqlite_path;
  const std::size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos || pos == 0) {
    return true;
  }
  const std::string dir = path.substr(0, pos);
#if defined(_WIN32)
  if (_mkdir(dir.c_str()) != 0) {
    // Ignore existing directory (EEXIST).
  }
#else
  mkdir(dir.c_str(), 0755);
#endif
  (void)error;
  return true;
}

void ServerApp::WireHandlers() {
  ingest_->set_upserted_handler([this](const std::vector<Video>& videos) {
    if (ws_.get() != NULL) {
      ws_->BroadcastUpserted(videos);
    }
  });
  health_->set_removed_handler(
      [this](const std::vector<std::int64_t>& ids) {
        if (ws_.get() != NULL) {
          ws_->BroadcastRemoved(ids);
        }
      });
}

bool ServerApp::Start(std::string* error) {
  if (started_) {
    if (error != NULL) {
      *error = "server already started";
    }
    return false;
  }

  if (!EnsureSqliteParentDir(error)) {
    return false;
  }

  repo_.reset(new SqliteVideoRepository());
  if (!repo_->Open(config_.sqlite_path, error)) {
    return false;
  }

  authorized_source_.reset(
      new AuthorizedCatalogSource(config_.authorized_catalog_path));
  archive_source_.reset(new ArchiveOrgSource());
  source_ptrs_.clear();
  source_ptrs_.push_back(authorized_source_.get());
  source_ptrs_.push_back(archive_source_.get());

  IngestConfig ingest_cfg = IngestConfig::MakeDefault();
  ingest_cfg.interval_ms = config_.ingest_interval_ms;
  ingest_cfg.keywords = config_.ingest_keywords;
  ingest_.reset(new IngestService(source_ptrs_, repo_.get(), ingest_cfg));

  prober_.reset(new UrlProber());
  UrlHealthConfig health_cfg = UrlHealthConfig::MakeDefault();
  health_cfg.interval_ms = config_.health_interval_ms;
  health_.reset(new UrlHealthService(repo_.get(), prober_.get(), health_cfg));

  http_.reset(new HttpApiServer(repo_.get(), ingest_.get()));
  ws_.reset(new WsPushServer());

  if (!http_->Start(config_.bind_host, config_.http_port, error)) {
    Stop();
    return false;
  }
  if (!ws_->Start(config_.bind_host, config_.ws_port, error)) {
    Stop();
    return false;
  }

  WireHandlers();

  // Initial catalog pull so search works immediately after boot.
  ingest_->runOnce();

  ingest_->start();
  health_->start();
  stop_requested_ = false;
  started_ = true;

  std::ostringstream ss;
  ss << "SquareDanceServer running; http=http://" << config_.bind_host << ":"
     << http_port() << " ws=ws://" << config_.bind_host << ":" << ws_port()
     << "/ws sqlite=" << config_.sqlite_path;
  LogInfo(ss.str());
  return true;
}

void ServerApp::RequestStop() {
  stop_requested_ = true;
  stop_cv_.notify_all();
}

void ServerApp::Stop() {
  if (!started_ && http_.get() == NULL && ws_.get() == NULL) {
    return;
  }

  if (ingest_.get() != NULL) {
    ingest_->stop();
  }
  if (health_.get() != NULL) {
    health_->stop();
  }
  if (http_.get() != NULL) {
    http_->Stop();
  }
  if (ws_.get() != NULL) {
    ws_->Stop();
  }
  if (repo_.get() != NULL) {
    repo_->Close();
  }

  http_.reset();
  ws_.reset();
  health_.reset();
  prober_.reset();
  ingest_.reset();
  source_ptrs_.clear();
  archive_source_.reset();
  authorized_source_.reset();
  repo_.reset();
  started_ = false;
  LogInfo("SquareDanceServer stopped");
}

int ServerApp::RunBlocking() {
  std::string error;
  if (!Start(&error)) {
    LogError("failed to start server: " + error);
    return 1;
  }

#if defined(_WIN32)
  g_console_app = this;
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif

  LogInfo("Press Ctrl+C to stop");
  {
    std::unique_lock<std::mutex> lock(stop_mutex_);
    stop_cv_.wait(lock, [this]() { return stop_requested_.load(); });
  }

#if defined(_WIN32)
  SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
  g_console_app = NULL;
#endif

  Stop();
  return 0;
}

}  // namespace server
}  // namespace square_dance
