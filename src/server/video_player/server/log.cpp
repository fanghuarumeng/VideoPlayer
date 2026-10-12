#include "video_player/server/log.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>

namespace video_player {
namespace server {
namespace {

std::mutex g_log_mutex;
std::string g_log_file;

const char* LevelName(LogLevel level) {
  switch (level) {
    case kLogWarn:
      return "WARN";
    case kLogError:
      return "ERROR";
    case kLogInfo:
    default:
      return "INFO";
  }
}

}  // namespace

void SetLogFilePath(const std::string& path) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  g_log_file = path;
}

void LogWrite(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  const std::string line =
      std::string("[") + LevelName(level) + "] " + message + "\n";
  std::cerr << line;
  if (!g_log_file.empty()) {
    std::ofstream out(g_log_file.c_str(), std::ios::app);
    if (out) {
      out << line;
    }
  }
}

}  // namespace server
}  // namespace video_player
