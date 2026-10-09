#include "square_dance/server/ServerConfig.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace square_dance {
namespace server {
namespace {

const int kTwelveHoursMs = 12 * 60 * 60 * 1000;

bool RequireString(const nlohmann::json& j,
                   const char* key,
                   std::string* out,
                   std::string* error) {
  if (!j.contains(key) || !j.at(key).is_string()) {
    if (error != NULL) {
      *error = std::string("missing or invalid string field: ") + key;
    }
    return false;
  }
  *out = j.at(key).get<std::string>();
  return true;
}

}  // namespace

ServerConfig MakeDefaultServerConfig() {
  ServerConfig cfg;
  cfg.bind_host = "127.0.0.1";
  cfg.http_port = 8080;
  cfg.ws_port = 8081;
  cfg.sqlite_path.clear();
  cfg.ingest_interval_ms = kTwelveHoursMs;
  cfg.health_interval_ms = kTwelveHoursMs;
  cfg.url_failure_threshold = 3;
  cfg.ingest_keywords.clear();
  cfg.ingest_keywords.push_back("广场舞");
  cfg.ingest_keywords.push_back("square dance");
  cfg.authorized_catalog_path.clear();
  return cfg;
}

bool LoadServerConfigFromString(const std::string& json_text,
                                ServerConfig* out,
                                std::string* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = "out is null";
    }
    return false;
  }

  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_text);
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = ex.what();
    }
    return false;
  }

  if (!j.is_object()) {
    if (error != NULL) {
      *error = "config root must be a JSON object";
    }
    return false;
  }

  ServerConfig cfg = MakeDefaultServerConfig();

  if (!RequireString(j, "sqlite_path", &cfg.sqlite_path, error)) {
    return false;
  }
  if (!RequireString(j, "authorized_catalog_path", &cfg.authorized_catalog_path,
                     error)) {
    return false;
  }

  if (j.contains("bind_host")) {
    if (!j.at("bind_host").is_string()) {
      if (error != NULL) {
        *error = "bind_host must be a string";
      }
      return false;
    }
    cfg.bind_host = j.at("bind_host").get<std::string>();
  }

  try {
    if (j.contains("http_port")) {
      cfg.http_port = static_cast<std::uint16_t>(j.at("http_port").get<int>());
    }
    if (j.contains("ws_port")) {
      cfg.ws_port = static_cast<std::uint16_t>(j.at("ws_port").get<int>());
    }
    if (j.contains("ingest_interval_ms")) {
      cfg.ingest_interval_ms = j.at("ingest_interval_ms").get<int>();
    }
    if (j.contains("health_interval_ms")) {
      cfg.health_interval_ms = j.at("health_interval_ms").get<int>();
    }
    if (j.contains("url_failure_threshold")) {
      cfg.url_failure_threshold = j.at("url_failure_threshold").get<int>();
    }
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = ex.what();
    }
    return false;
  }

  if (j.contains("ingest_keywords")) {
    if (!j.at("ingest_keywords").is_array()) {
      if (error != NULL) {
        *error = "ingest_keywords must be an array";
      }
      return false;
    }
    cfg.ingest_keywords.clear();
    for (nlohmann::json::const_iterator it = j.at("ingest_keywords").begin();
         it != j.at("ingest_keywords").end(); ++it) {
      if (!it->is_string()) {
        if (error != NULL) {
          *error = "ingest_keywords entries must be strings";
        }
        return false;
      }
      cfg.ingest_keywords.push_back(it->get<std::string>());
    }
  }

  *out = cfg;
  return true;
}

bool LoadServerConfigFromFile(const std::string& path,
                              ServerConfig* out,
                              std::string* error) {
  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  if (!in) {
    if (error != NULL) {
      *error = "failed to open config file: " + path;
    }
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return LoadServerConfigFromString(ss.str(), out, error);
}

}  // namespace server
}  // namespace square_dance
