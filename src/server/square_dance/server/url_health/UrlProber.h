#pragma once

#include "square_dance/server/url_health/IUrlProber.h"

namespace square_dance {
namespace server {

// HEAD with short GET fallback. HTTPS requires OpenSSL-enabled httplib.
class UrlProber : public IUrlProber {
 public:
  virtual bool isReachable(const std::string& url, std::string* error);
};

}  // namespace server
}  // namespace square_dance
