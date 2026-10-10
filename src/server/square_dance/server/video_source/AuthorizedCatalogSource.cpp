#include "square_dance/server/video_source/AuthorizedCatalogSource.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace square_dance {
namespace server {
namespace {

void SetError(std::string* error, const std::string& message) {
  if (error != NULL) {
    *error = message;
  }
}

std::string ToLowerAscii(std::string value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(value[i]);
    value[i] = static_cast<char>(std::tolower(ch));
  }
  return value;
}

bool IsAllowedMediaUrl(const std::string& url) {
  const std::string lower = ToLowerAscii(url);
  return lower.compare(0, 7, "http://") == 0 ||
         lower.compare(0, 8, "https://") == 0 ||
         lower.compare(0, 7, "file://") == 0;
}

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle) {
  if (needle.empty()) {
    return true;
  }
  const std::string h = ToLowerAscii(haystack);
  const std::string n = ToLowerAscii(needle);
  return h.find(n) != std::string::npos;
}

}  // namespace

AuthorizedCatalogSource::AuthorizedCatalogSource(const std::string& catalog_path)
    : catalog_path_(catalog_path) {}

std::string AuthorizedCatalogSource::id() const {
  return "authorized";
}

bool AuthorizedCatalogSource::ParseCatalogJson(const std::string& json_text,
                                               std::vector<VideoCandidate>* out,
                                               std::string* error) {
  if (out == NULL) {
    SetError(error, "out is null");
    return false;
  }
  out->clear();

  nlohmann::json root;
  try {
    root = nlohmann::json::parse(json_text);
  } catch (const std::exception& ex) {
    SetError(error, ex.what());
    return false;
  }

  if (!root.is_object() || !root.contains("videos") ||
      !root.at("videos").is_array()) {
    SetError(error, "catalog must be an object with a videos array");
    return false;
  }

  const nlohmann::json& videos = root.at("videos");
  for (nlohmann::json::const_iterator it = videos.begin(); it != videos.end();
       ++it) {
    if (!it->is_object()) {
      continue;
    }
    const nlohmann::json& item = *it;
    if (!item.contains("id") || !item.at("id").is_string() ||
        !item.contains("title") || !item.at("title").is_string()) {
      continue;
    }

    std::string play_url;
    std::string download_url;
    if (item.contains("play_url") && item.at("play_url").is_string()) {
      play_url = item.at("play_url").get<std::string>();
    }
    if (item.contains("download_url") && item.at("download_url").is_string()) {
      download_url = item.at("download_url").get<std::string>();
    }
    if (download_url.empty()) {
      download_url = play_url;
    }
    if (play_url.empty()) {
      play_url = download_url;
    }
    if (play_url.empty() || download_url.empty()) {
      continue;
    }
    if (!IsAllowedMediaUrl(play_url) || !IsAllowedMediaUrl(download_url)) {
      continue;
    }

    VideoCandidate c;
    c.external_id = item.at("id").get<std::string>();
    c.title = item.at("title").get<std::string>();
    c.play_url = play_url;
    c.download_url = download_url;
    c.source_type = "authorized";
    out->push_back(c);
  }
  return true;
}

std::vector<VideoCandidate> AuthorizedCatalogSource::search(
    const std::string& keyword, int limit, std::string* error) {
  std::vector<VideoCandidate> empty;
  if (limit <= 0) {
    return empty;
  }

  std::ifstream in(catalog_path_.c_str(), std::ios::in | std::ios::binary);
  if (!in) {
    SetError(error, "failed to open authorized catalog: " + catalog_path_);
    return empty;
  }
  std::ostringstream ss;
  ss << in.rdbuf();

  std::vector<VideoCandidate> all;
  if (!ParseCatalogJson(ss.str(), &all, error)) {
    return empty;
  }

  std::vector<VideoCandidate> matched;
  for (std::size_t i = 0; i < all.size(); ++i) {
    if (!ContainsIgnoreCase(all[i].title, keyword)) {
      continue;
    }
    matched.push_back(all[i]);
    if (static_cast<int>(matched.size()) >= limit) {
      break;
    }
  }
  return matched;
}

}  // namespace server
}  // namespace square_dance
