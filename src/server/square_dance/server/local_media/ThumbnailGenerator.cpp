#include "square_dance/server/local_media/ThumbnailGenerator.h"

#include "square_dance/server/log.h"

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

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace square_dance {
namespace server {

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

  // Extract a frame near 1s (falls back near start for short clips).
  std::ostringstream cmd;
#if defined(_WIN32)
  cmd << "\"" << ffmpeg_path_ << "\" -y -ss 1 -i \"" << job.video_path
      << "\" -frames:v 1 -q:v 2 \"" << job.thumb_path << "\"";
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  std::string cmdline = cmd.str();
  std::vector<char> mutable_cmd(cmdline.begin(), cmdline.end());
  mutable_cmd.push_back('\0');
  if (!CreateProcessA(NULL, &mutable_cmd[0], NULL, NULL, FALSE,
                      CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
    if (error != NULL) {
      *error = "CreateProcess failed for ffmpeg";
    }
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD code = 1;
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if (code != 0) {
    if (error != NULL) {
      *error = "ffmpeg exited with non-zero status";
    }
    return false;
  }
#else
  cmd << "\"" << ffmpeg_path_ << "\" -y -ss 1 -i \"" << job.video_path
      << "\" -frames:v 1 -q:v 2 \"" << job.thumb_path << "\" 2>/dev/null";
  const int rc = std::system(cmd.str().c_str());
  if (rc != 0) {
    if (error != NULL) {
      *error = "ffmpeg exited with non-zero status";
    }
    return false;
  }
#endif
  LogInfo("thumbnail generated for video_id=" +
          std::to_string(static_cast<long long>(job.video_id)));
  return true;
}

}  // namespace server
}  // namespace square_dance
