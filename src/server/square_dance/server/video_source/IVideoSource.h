#pragma once

#include "square_dance/server/video_source/VideoCandidate.h"

#include <string>
#include <vector>

namespace square_dance {
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
}  // namespace square_dance
