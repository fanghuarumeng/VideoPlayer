#pragma once

#include <string>

namespace square_dance {
namespace server {

class IUrlProber {
 public:
  virtual ~IUrlProber() {}
  virtual bool isReachable(const std::string& url, std::string* error) = 0;
};

}  // namespace server
}  // namespace square_dance
