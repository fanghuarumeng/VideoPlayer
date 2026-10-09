#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace square_dance {
namespace server {

struct ServerConfig {
  std::string bind_host;
  std::uint16_t http_port;
  std::uint16_t ws_port;
  std::string sqlite_path;
  int ingest_interval_ms;
  int health_interval_ms;
  int url_failure_threshold;
  std::vector<std::string> ingest_keywords;
  std::string authorized_catalog_path;
};

ServerConfig MakeDefaultServerConfig();

bool LoadServerConfigFromString(const std::string& json_text,
                                ServerConfig* out,
                                std::string* error);

bool LoadServerConfigFromFile(const std::string& path,
                              ServerConfig* out,
                              std::string* error);

}  // namespace server
}  // namespace square_dance
