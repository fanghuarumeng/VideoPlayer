#pragma once

#include "video_player/VideoDto.h"
#include "video_player/server/video_store/Video.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace video_player {
namespace server {

VideoDto ToVideoDto(const Video& video);
nlohmann::json VideoToApiJson(const Video& video);
nlohmann::json VideoListToApiJson(const std::vector<Video>& videos);

}  // namespace server
}  // namespace video_player
