#pragma once

#include "video_player/server/video_source/VideoCandidate.h"

#include <string>
#include <vector>

namespace video_player {
namespace server {

class IVideoSource {
 public:
  virtual ~IVideoSource() {}
  virtual std::string id() const = 0;
  virtual std::vector<VideoCandidate> search(const std::string& keyword,
                                             int limit,
                                             std::string* error) = 0;
};

}  // namespace server
}  // namespace video_player
