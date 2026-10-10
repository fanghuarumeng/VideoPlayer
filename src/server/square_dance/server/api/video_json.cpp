#include "square_dance/server/api/video_json.h"

#include "square_dance/video_json.h"

namespace square_dance {
namespace server {

VideoDto ToVideoDto(const Video& video) {
  VideoDto dto;
  dto.id = video.id;
  dto.external_id = video.external_id;
  dto.title = video.title;
  dto.play_url = video.play_url;
  dto.download_url = video.download_url;
  dto.source_type = video.source_type;
  dto.updated_at = video.updated_at;
  dto.thumbnail_url = video.thumbnail_url;
  return dto;
}

nlohmann::json VideoToApiJson(const Video& video) {
  return videoDtoToJson(ToVideoDto(video));
}

nlohmann::json VideoListToApiJson(const std::vector<Video>& videos) {
  nlohmann::json arr = nlohmann::json::array();
  for (std::size_t i = 0; i < videos.size(); ++i) {
    arr.push_back(VideoToApiJson(videos[i]));
  }
  return arr;
}

}  // namespace server
}  // namespace square_dance
