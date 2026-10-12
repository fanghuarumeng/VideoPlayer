#pragma once

#include <string>

namespace video_player {
namespace server {

class IUrlProber {
 public:
  virtual ~IUrlProber() {}
  virtual bool isReachable(const std::string& url, std::string* error) = 0;
};

}  // namespace server
}  // namespace video_player
