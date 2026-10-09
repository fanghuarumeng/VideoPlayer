#include "square_dance/server/video_source/ArchiveOrgSource.h"

#include <nlohmann/json.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <cctype>
#include <sstream>

namespace square_dance {
namespace server {
namespace {

void SetError(std::string* error, const std::string& message) {
  if (error != NULL) {
    *error = message;
  }
}

std::string UrlEncode(const std::string& value) {
  static const char* kHex = "0123456789ABCDEF";
  std::string out;
  out.reserve(value.size() * 3);
  for (std::size_t i = 0; i < value.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(value[i]);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~') {
      out.push_back(static_cast<char>(c));
    } else if (c == ' ') {
      out.push_back('+');
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0x0F]);
    }
  }
  return out;
}

std::string ToLower(std::string value) {
  for (std::size_t i = 0; i < value.size(); ++i) {
    value[i] = static_cast<char>(
        std::tolower(static_cast<unsigned char>(value[i])));
  }
  return value;
}

bool EndsWith(const std::string& value, const std::string& suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
         0;
}

bool IsVideoFileName(const std::string& name) {
  const std::string lower = ToLower(name);
  return EndsWith(lower, ".mp4") || EndsWith(lower, ".webm") ||
         EndsWith(lower, ".ogv") || EndsWith(lower, ".avi") ||
         EndsWith(lower, ".mkv") || EndsWith(lower, ".mov");
}

bool DefaultHttpGet(const std::string& url,
                    std::string* body,
                    std::string* error) {
  if (body == NULL) {
    SetError(error, "body is null");
    return false;
  }

  // Archive.org is HTTPS. cpp-httplib needs OpenSSL for HTTPS; without it this
  // fails with a clear error. Tests inject fixtures via HttpGetFn.
  const std::string https = "https://";
  if (url.compare(0, https.size(), https) != 0) {
    SetError(error, "only https archive.org URLs are supported");
    return false;
  }

  const std::string rest = url.substr(https.size());
  const std::size_t slash = rest.find('/');
  const std::string host =
      slash == std::string::npos ? rest : rest.substr(0, slash);
  const std::string path =
      slash == std::string::npos ? "/" : rest.substr(slash);

#if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
  httplib::SSLClient client(host.c_str());
  client.set_connection_timeout(10, 0);
  client.set_read_timeout(20, 0);
  const httplib::Result res = client.Get(path.c_str());
  if (!res) {
    SetError(error, "archive.org HTTP request failed");
    return false;
  }
  if (res->status != 200) {
    std::ostringstream ss;
    ss << "archive.org HTTP status " << res->status;
    SetError(error, ss.str());
    return false;
  }
  *body = res->body;
  return true;
#else
  (void)host;
  (void)path;
  SetError(error,
           "HTTPS fetch requires OpenSSL-enabled cpp-httplib; "
           "use injected HttpGetFn or enable CPPHTTPLIB_OPENSSL_SUPPORT");
  return false;
#endif
}

}  // namespace

ArchiveOrgSource::ArchiveOrgSource() : http_get_(&DefaultHttpGet) {}

ArchiveOrgSource::ArchiveOrgSource(HttpGetFn http_get)
    : http_get_(http_get) {}

std::string ArchiveOrgSource::id() const {
  return "archive_org";
}

std::string ArchiveOrgSource::BuildSearchUrl(const std::string& keyword,
                                             int rows) {
  if (rows <= 0) {
    rows = 1;
  }
  std::ostringstream q;
  q << "(" << keyword << ") AND mediatype:(movies)";
  std::ostringstream url;
  url << "https://archive.org/advancedsearch.php?q=" << UrlEncode(q.str())
      << "&fl[]=identifier&fl[]=title&rows=" << rows
      << "&page=1&output=json";
  return url.str();
}

std::string ArchiveOrgSource::BuildDownloadUrl(const std::string& identifier,
                                               const std::string& file_name) {
  return "https://archive.org/download/" + identifier + "/" + file_name;
}

bool ArchiveOrgSource::ParseSearchResponse(
    const std::string& json_text,
    std::vector<ArchiveOrgSearchHit>* out,
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

  if (!root.is_object() || !root.contains("response") ||
      !root.at("response").is_object() ||
      !root.at("response").contains("docs") ||
      !root.at("response").at("docs").is_array()) {
    SetError(error, "invalid archive.org search response");
    return false;
  }

  const nlohmann::json& docs = root.at("response").at("docs");
  for (nlohmann::json::const_iterator it = docs.begin(); it != docs.end();
       ++it) {
    if (!it->is_object() || !it->contains("identifier")) {
      continue;
    }
    ArchiveOrgSearchHit hit;
    if (it->at("identifier").is_string()) {
      hit.identifier = it->at("identifier").get<std::string>();
    } else if (it->at("identifier").is_array() &&
               !it->at("identifier").empty() &&
               it->at("identifier")[0].is_string()) {
      hit.identifier = it->at("identifier")[0].get<std::string>();
    }
    if (hit.identifier.empty()) {
      continue;
    }
    if (it->contains("title")) {
      if (it->at("title").is_string()) {
        hit.title = it->at("title").get<std::string>();
      } else if (it->at("title").is_array() && !it->at("title").empty() &&
                 it->at("title")[0].is_string()) {
        hit.title = it->at("title")[0].get<std::string>();
      }
    }
    if (hit.title.empty()) {
      hit.title = hit.identifier;
    }
    out->push_back(hit);
  }
  return true;
}

std::string ArchiveOrgSource::PickMediaFileName(const std::string& metadata_json,
                                                std::string* error) {
  nlohmann::json root;
  try {
    root = nlohmann::json::parse(metadata_json);
  } catch (const std::exception& ex) {
    SetError(error, ex.what());
    return "";
  }

  if (!root.is_object() || !root.contains("files") ||
      !root.at("files").is_array()) {
    SetError(error, "invalid archive.org metadata (missing files)");
    return "";
  }

  std::string fallback;
  const nlohmann::json& files = root.at("files");
  for (nlohmann::json::const_iterator it = files.begin(); it != files.end();
       ++it) {
    if (!it->is_object() || !it->contains("name") ||
        !it->at("name").is_string()) {
      continue;
    }
    const std::string name = it->at("name").get<std::string>();
    if (!IsVideoFileName(name)) {
      continue;
    }
    if (EndsWith(ToLower(name), ".mp4")) {
      return name;
    }
    if (fallback.empty()) {
      fallback = name;
    }
  }
  if (fallback.empty()) {
    SetError(error, "no downloadable video file in metadata");
  }
  return fallback;
}

std::vector<VideoCandidate> ArchiveOrgSource::search(const std::string& keyword,
                                                     int limit,
                                                     std::string* error) {
  std::vector<VideoCandidate> empty;
  if (limit <= 0) {
    return empty;
  }
  if (!http_get_) {
    SetError(error, "http getter is not set");
    return empty;
  }

  std::string search_body;
  const std::string search_url = BuildSearchUrl(keyword, limit);
  if (!http_get_(search_url, &search_body, error)) {
    return empty;
  }

  std::vector<ArchiveOrgSearchHit> hits;
  if (!ParseSearchResponse(search_body, &hits, error)) {
    return empty;
  }

  std::vector<VideoCandidate> out;
  for (std::size_t i = 0; i < hits.size() && static_cast<int>(out.size()) < limit;
       ++i) {
    const ArchiveOrgSearchHit& hit = hits[i];
    const std::string meta_url =
        "https://archive.org/metadata/" + hit.identifier;
    std::string meta_body;
    std::string meta_error;
    if (!http_get_(meta_url, &meta_body, &meta_error)) {
      continue;
    }
    std::string pick_error;
    const std::string file_name = PickMediaFileName(meta_body, &pick_error);
    if (file_name.empty()) {
      continue;
    }

    VideoCandidate c;
    c.external_id = hit.identifier;
    c.title = hit.title;
    c.play_url = BuildDownloadUrl(hit.identifier, file_name);
    c.download_url = c.play_url;
    c.source_type = "archive_org";
    out.push_back(c);
  }
  return out;
}

}  // namespace server
}  // namespace square_dance
