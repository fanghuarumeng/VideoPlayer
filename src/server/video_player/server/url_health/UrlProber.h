#pragma once

#include "video_player/server/url_health/IUrlProber.h"

namespace video_player {
namespace server {

// HEAD with short GET fallback. HTTPS requires OpenSSL-enabled httplib.
class UrlProber : public IUrlProber {
 public:
  virtual bool isReachable(const std::string& url, std::string* error);
};

}  // namespace server
}  // namespace video_player
