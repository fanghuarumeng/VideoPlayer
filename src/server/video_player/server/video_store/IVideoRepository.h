#pragma once

#include "video_player/server/video_store/Video.h"

#include <string>
#include <vector>

namespace video_player {
namespace server {

class IVideoRepository {
 public:
  virtual ~IVideoRepository() {}

  virtual bool upsert(const Video& video, std::string* error) = 0;
  virtual std::vector<Video> searchByName(const std::string& query,
                                          int limit) = 0;
  virtual bool getById(std::int64_t id, Video* out, std::string* error) = 0;
  virtual bool findBySourceKey(const std::string& source_type,
                               const std::string& external_id,
                               Video* out,
                               std::string* error) = 0;
  virtual bool removeById(std::int64_t id, std::string* error) = 0;
  virtual std::vector<Video> listAll(int limit, int offset) = 0;
};

}  // namespace server
}  // namespace video_player
