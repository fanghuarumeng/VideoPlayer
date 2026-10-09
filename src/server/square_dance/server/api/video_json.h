#pragma once

#include "square_dance/VideoDto.h"
#include "square_dance/server/video_store/Video.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace square_dance {
namespace server {

VideoDto ToVideoDto(const Video& video);
nlohmann::json VideoToApiJson(const Video& video);
nlohmann::json VideoListToApiJson(const std::vector<Video>& videos);

}  // namespace server
}  // namespace square_dance
