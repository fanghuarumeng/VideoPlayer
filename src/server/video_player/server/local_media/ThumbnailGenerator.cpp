#include "video_player/server/local_media/ThumbnailGenerator.h"

#include "video_player/server/log.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace video_player {
namespace server {
namespace {

bool FileNonEmpty(const std::string& path) {
  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  if (!in) {
    return false;
  }
  in.seekg(0, std::ios::end);
  return in.tellg() > 0;
}

}  // namespace

FfmpegThumbnailGenerator::FfmpegThumbnailGenerator(
    const std::string& ffmpeg_path)
    : ffmpeg_path_(ffmpeg_path.empty() ? "ffmpeg" : ffmpeg_path) {}

bool FfmpegThumbnailGenerator::generate(const ThumbJob& job,
                                        std::string* error) {
  if (job.video_path.empty() || job.thumb_path.empty()) {
    if (error != NULL) {
      *error = "thumb job paths empty";
    }
    return false;
  }

  // -update 1: required by recent ffmpeg image2 muxer for a single .jpg path.
  // -an: skip audio (MXF multi-track audio can confuse some builds).
  std::ostringstream cmd;
#if defined(_WIN32)
  cmd << "\"" << ffmpeg_path_ << "\" -y -ss 1 -i \"" << job.video_path
      << "\" -an -frames:v 1 -update 1 -q:v 2 \"" << job.thumb_path << "\"";
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  std::string cmdline = cmd.str();
  std::vector<char> mutable_cmd(cmdline.begin(), cmdline.end());
  mutable_cmd.push_back('\0');

  // Prefer explicit exe path so we do not depend on PATH (common when the
  // server is launched from an IDE / Start-Process without vendor ffmpeg).
  const char* app =
      (ffmpeg_path_.find('\\') != std::string::npos ||
       ffmpeg_path_.find('/') != std::string::npos)
          ? ffmpeg_path_.c_str()
          : NULL;

  if (!CreateProcessA(app, &mutable_cmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW,
                      NULL, NULL, &si, &pi)) {
    if (error != NULL) {
      *error = "CreateProcess failed for ffmpeg (check ffmpeg_path in config)";
    }
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 1;
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if (!FileNonEmpty(job.thumb_path)) {
    if (error != NULL) {
      std::ostringstream ss;
      ss << "ffmpeg produced no thumbnail (exit=" << code << ")";
      *error = ss.str();
    }
    return false;
  }
#else
  cmd << "\"" << ffmpeg_path_ << "\" -y -ss 1 -i \"" << job.video_path
      << "\" -an -frames:v 1 -update 1 -q:v 2 \"" << job.thumb_path
      << "\" 2>/dev/null";
  const int rc = std::system(cmd.str().c_str());
  if (!FileNonEmpty(job.thumb_path)) {
    if (error != NULL) {
      std::ostringstream ss;
      ss << "ffmpeg produced no thumbnail (rc=" << rc << ")";
      *error = ss.str();
    }
    return false;
  }
#endif
  LogInfo("thumbnail generated for video_id=" +
          std::to_string(static_cast<long long>(job.video_id)));
  return true;
}

}  // namespace server
}  // namespace video_player
