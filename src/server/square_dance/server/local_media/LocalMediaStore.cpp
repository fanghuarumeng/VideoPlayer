#include "square_dance/server/local_media/LocalMediaStore.h"

#include "square_dance/server/log.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

namespace square_dance {
namespace server {
namespace {

std::atomic<std::uint64_t> g_upload_seq(0);

void MkDirIgnoreExists(const std::string& path) {
#if defined(_WIN32)
  _mkdir(path.c_str());
#else
  mkdir(path.c_str(), 0755);
#endif
}

std::string ToLower(std::string s) {
  for (std::size_t i = 0; i < s.size(); ++i) {
    s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
  }
  return s;
}

std::string NowUtcIso() {
  const std::time_t now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm tm_buf;
#if defined(_WIN32)
  gmtime_s(&tm_buf, &now);
#else
  gmtime_r(&now, &tm_buf);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
  return std::string(buf);
}

std::string BasenameOnly(const std::string& name) {
  const std::size_t pos = name.find_last_of("/\\");
  if (pos == std::string::npos) {
    return name;
  }
  return name.substr(pos + 1);
}

}  // namespace

LocalMediaStore::LocalMediaStore(IVideoRepository* repo,
                                 const std::string& media_root)
    : repo_(repo), media_root_(media_root), http_base_() {}

void LocalMediaStore::set_http_base(const std::string& http_base) {
  http_base_ = http_base;
  // Drop trailing slash.
  while (!http_base_.empty() &&
         (http_base_[http_base_.size() - 1] == '/' ||
          http_base_[http_base_.size() - 1] == '\\')) {
    http_base_.resize(http_base_.size() - 1);
  }
}

std::string LocalMediaStore::media_root() const {
  return media_root_;
}

std::string LocalMediaStore::videos_dir() const {
  return media_root_ + "/videos";
}

std::string LocalMediaStore::ExtensionOf(const std::string& filename) {
  const std::string base = BasenameOnly(filename);
  const std::size_t dot = base.find_last_of('.');
  if (dot == std::string::npos || dot + 1 >= base.size()) {
    return std::string();
  }
  return ToLower(base.substr(dot));
}

std::string LocalMediaStore::StemOf(const std::string& filename) {
  const std::string base = BasenameOnly(filename);
  const std::size_t dot = base.find_last_of('.');
  if (dot == std::string::npos) {
    return base;
  }
  return base.substr(0, dot);
}

bool LocalMediaStore::IsAllowedExtension(const std::string& filename) {
  const std::string ext = ExtensionOf(filename);
  return ext == ".mp4" || ext == ".mkv" || ext == ".webm" || ext == ".avi" ||
         ext == ".mov" || ext == ".mxf";
}

std::string LocalMediaStore::ContentTypeForExtension(const std::string& ext) {
  const std::string e = ToLower(ext);
  if (e == ".mp4") {
    return "video/mp4";
  }
  if (e == ".mkv") {
    return "video/x-matroska";
  }
  if (e == ".webm") {
    return "video/webm";
  }
  if (e == ".avi") {
    return "video/x-msvideo";
  }
  if (e == ".mov") {
    return "video/quicktime";
  }
  if (e == ".mxf") {
    return "application/mxf";
  }
  return "application/octet-stream";
}

bool LocalMediaStore::EnsureDirs(std::string* error) {
  if (media_root_.empty()) {
    if (error != NULL) {
      *error = "media_root is empty";
    }
    return false;
  }
  MkDirIgnoreExists(media_root_);
  MkDirIgnoreExists(videos_dir());
  MkDirIgnoreExists(media_root_ + "/thumbs");
  return true;
}

std::string LocalMediaStore::MakeExternalId() const {
  const std::uint64_t seq = ++g_upload_seq;
  const std::time_t now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream ss;
  ss << "local-" << static_cast<long long>(now) << "-" << seq;
  return ss.str();
}

bool LocalMediaStore::storeUpload(const std::string& original_filename,
                                  const std::string& title,
                                  const std::string& content,
                                  Video* out,
                                  std::string* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = "out is null";
    }
    return false;
  }
  if (repo_ == NULL) {
    if (error != NULL) {
      *error = "repository unavailable";
    }
    return false;
  }
  if (http_base_.empty()) {
    if (error != NULL) {
      *error = "http_base not set";
    }
    return false;
  }
  if (!IsAllowedExtension(original_filename)) {
    if (error != NULL) {
      *error = "unsupported file extension";
    }
    return false;
  }
  if (content.empty()) {
    if (error != NULL) {
      *error = "empty file";
    }
    return false;
  }
  if (!EnsureDirs(error)) {
    return false;
  }

  const std::string ext = ExtensionOf(original_filename);
  const std::string external_id = MakeExternalId();
  const std::string file_name = external_id + ext;
  const std::string dest_path = videos_dir() + "/" + file_name;

  {
    std::ofstream out_file(dest_path.c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out_file) {
      if (error != NULL) {
        *error = "failed to open destination file";
      }
      return false;
    }
    out_file.write(content.data(),
                   static_cast<std::streamsize>(content.size()));
    if (!out_file) {
      if (error != NULL) {
        *error = "failed to write destination file";
      }
      std::remove(dest_path.c_str());
      return false;
    }
  }

  const std::string now = NowUtcIso();
  std::string resolved_title = title;
  if (resolved_title.empty()) {
    resolved_title = StemOf(original_filename);
  }
  if (resolved_title.empty()) {
    resolved_title = external_id;
  }

  Video video;
  video.id = 0;
  video.external_id = external_id;
  video.title = resolved_title;
  video.play_url = http_base_ + "/media/videos/" + file_name;
  video.download_url = video.play_url;
  video.source_type = "local_upload";
  video.consecutive_failures = 0;
  video.created_at = now;
  video.updated_at = now;
  video.thumbnail_url.clear();

  if (!repo_->upsert(video, error)) {
    std::remove(dest_path.c_str());
    return false;
  }
  if (!repo_->findBySourceKey("local_upload", external_id, out, error)) {
    return false;
  }
  LogInfo("local-media stored " + file_name + " id=" +
          std::to_string(static_cast<long long>(out->id)));
  return true;
}

}  // namespace server
}  // namespace square_dance
