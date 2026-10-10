#pragma once

#include "square_dance/server/video_store/IVideoRepository.h"
#include "square_dance/server/video_store/Video.h"

#include <string>

namespace square_dance {
namespace server {

class LocalMediaStore {
 public:
  LocalMediaStore(IVideoRepository* repo, const std::string& media_root);

  void set_http_base(const std::string& http_base);

  // Writes bytes under media_root/videos/, upserts DB row (thumbnail empty).
  bool storeUpload(const std::string& original_filename,
                   const std::string& title,
                   const std::string& content,
                   Video* out,
                   std::string* error);

  std::string media_root() const;
  std::string videos_dir() const;

  static bool IsAllowedExtension(const std::string& filename);
  static std::string ExtensionOf(const std::string& filename);
  static std::string StemOf(const std::string& filename);
  static std::string ContentTypeForExtension(const std::string& ext);

 private:
  LocalMediaStore(const LocalMediaStore&);
  LocalMediaStore& operator=(const LocalMediaStore&);

  bool EnsureDirs(std::string* error);
  std::string MakeExternalId() const;

  IVideoRepository* repo_;
  std::string media_root_;
  std::string http_base_;
};

}  // namespace server
}  // namespace square_dance
