#pragma once

#include "video_player/server/video_source/IVideoSource.h"

#include <functional>
#include <string>
#include <vector>

namespace video_player {
namespace server {

struct ArchiveOrgSearchHit {
  std::string identifier;
  std::string title;
};

class ArchiveOrgSource : public IVideoSource {
 public:
  // GET url -> body. Return false and set error on failure.
  typedef std::function<bool(const std::string& url,
                             std::string* body,
                             std::string* error)>
      HttpGetFn;

  ArchiveOrgSource();
  explicit ArchiveOrgSource(HttpGetFn http_get);

  virtual std::string id() const;
  virtual std::vector<VideoCandidate> search(const std::string& keyword,
                                             int limit,
                                             std::string* error);

  static bool ParseSearchResponse(const std::string& json_text,
                                  std::vector<ArchiveOrgSearchHit>* out,
                                  std::string* error);

  // Prefer .mp4; otherwise first common video extension. Empty if none.
  static std::string PickMediaFileName(const std::string& metadata_json,
                                       std::string* error);

  static std::string BuildDownloadUrl(const std::string& identifier,
                                      const std::string& file_name);

  static std::string BuildSearchUrl(const std::string& keyword, int rows);

 private:
  HttpGetFn http_get_;
};

}  // namespace server
}  // namespace video_player
