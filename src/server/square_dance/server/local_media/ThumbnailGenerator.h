#pragma once

#include <cstdint>
#include <string>

namespace square_dance {
namespace server {

struct ThumbJob {
  std::int64_t video_id;
  std::string video_path;
  std::string thumb_path;
  std::string thumb_filename;  // e.g. local-123.jpg for URL path
};

class IThumbnailGenerator {
 public:
  virtual ~IThumbnailGenerator() {}
  virtual bool generate(const ThumbJob& job, std::string* error) = 0;
};

class FfmpegThumbnailGenerator : public IThumbnailGenerator {
 public:
  explicit FfmpegThumbnailGenerator(const std::string& ffmpeg_path);
  virtual bool generate(const ThumbJob& job, std::string* error);

 private:
  std::string ffmpeg_path_;
};

}  // namespace server
}  // namespace square_dance
