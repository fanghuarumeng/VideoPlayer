#include "square_dance/server/ServerApp.h"

#include "square_dance/server/log.h"

#include <sstream>

namespace square_dance {
namespace server {

ServerApp::ServerApp(const ServerConfig& config) : config_(config) {}

const ServerConfig& ServerApp::config() const {
  return config_;
}

int ServerApp::RunShellOnce() {
  std::ostringstream ss;
  ss << "SquareDanceServer shell ready; bind=" << config_.bind_host
     << " http=" << config_.http_port << " ws=" << config_.ws_port
     << " sqlite=" << config_.sqlite_path
     << " catalog=" << config_.authorized_catalog_path
     << " ingest_ms=" << config_.ingest_interval_ms
     << " health_ms=" << config_.health_interval_ms
     << " fail_threshold=" << config_.url_failure_threshold;
  LogInfo(ss.str());
  return 0;
}

}  // namespace server
}  // namespace square_dance
