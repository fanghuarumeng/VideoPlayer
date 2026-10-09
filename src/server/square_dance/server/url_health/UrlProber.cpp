#include "square_dance/server/url_health/UrlProber.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <sstream>

namespace square_dance {
namespace server {
namespace {

void SetError(std::string* error, const std::string& message) {
  if (error != NULL) {
    *error = message;
  }
}

bool ParseUrl(const std::string& url,
              bool* https,
              std::string* host,
              std::string* path,
              std::string* error) {
  const std::string https_prefix = "https://";
  const std::string http_prefix = "http://";
  std::string rest;
  if (url.compare(0, https_prefix.size(), https_prefix) == 0) {
    *https = true;
    rest = url.substr(https_prefix.size());
  } else if (url.compare(0, http_prefix.size(), http_prefix) == 0) {
    *https = false;
    rest = url.substr(http_prefix.size());
  } else {
    SetError(error, "unsupported URL scheme");
    return false;
  }

  const std::size_t slash = rest.find('/');
  *host = slash == std::string::npos ? rest : rest.substr(0, slash);
  *path = slash == std::string::npos ? "/" : rest.substr(slash);
  if (host->empty()) {
    SetError(error, "empty host");
    return false;
  }
  return true;
}

template <typename ClientT>
bool ProbeWithClient(ClientT& client,
                     const std::string& path,
                     std::string* error) {
  client.set_connection_timeout(5, 0);
  client.set_read_timeout(5, 0);

  httplib::Result head = client.Head(path.c_str());
  if (head) {
    if (head->status >= 200 && head->status < 400) {
      return true;
    }
    // Some hosts disallow HEAD; fall through to GET.
    if (head->status != 405 && head->status != 501) {
      std::ostringstream ss;
      ss << "HEAD status " << head->status;
      SetError(error, ss.str());
      return false;
    }
  }

  httplib::Headers headers;
  headers.emplace("Range", "bytes=0-0");
  httplib::Result get = client.Get(path.c_str(), headers);
  if (!get) {
    SetError(error, "GET probe failed");
    return false;
  }
  if (get->status >= 200 && get->status < 400) {
    return true;
  }
  std::ostringstream ss;
  ss << "GET status " << get->status;
  SetError(error, ss.str());
  return false;
}

}  // namespace

bool UrlProber::isReachable(const std::string& url, std::string* error) {
  bool https = false;
  std::string host;
  std::string path;
  if (!ParseUrl(url, &https, &host, &path, error)) {
    return false;
  }

  if (https) {
#if defined(CPPHTTPLIB_OPENSSL_SUPPORT)
    httplib::SSLClient client(host.c_str());
    return ProbeWithClient(client, path, error);
#else
    SetError(error,
             "HTTPS probe requires OpenSSL-enabled cpp-httplib "
             "(CPPHTTPLIB_OPENSSL_SUPPORT)");
    return false;
#endif
  }

  httplib::Client client(host.c_str());
  return ProbeWithClient(client, path, error);
}

}  // namespace server
}  // namespace square_dance
