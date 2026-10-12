#pragma once

#include "video_player/server/video_source/IVideoSource.h"

#include <string>

namespace video_player {
namespace server {

class AuthorizedCatalogSource : public IVideoSource {
 public:
  explicit AuthorizedCatalogSource(const std::string& catalog_path);

  virtual std::string id() const;
  virtual std::vector<VideoCandidate> search(const std::string& keyword,
                                             int limit,
                                             std::string* error);

  // Test helper: parse catalog JSON text without touching disk.
  static bool ParseCatalogJson(const std::string& json_text,
                               std::vector<VideoCandidate>* out,
                               std::string* error);

 private:
  std::string catalog_path_;
};

}  // namespace server
}  // namespace video_player
