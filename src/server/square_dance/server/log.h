#pragma once

#include <string>

namespace square_dance {
namespace server {

enum LogLevel {
  kLogInfo = 0,
  kLogWarn = 1,
  kLogError = 2
};

void SetLogFilePath(const std::string& path);
void LogWrite(LogLevel level, const std::string& message);

inline void LogInfo(const std::string& message) {
  LogWrite(kLogInfo, message);
}

inline void LogWarn(const std::string& message) {
  LogWrite(kLogWarn, message);
}

inline void LogError(const std::string& message) {
  LogWrite(kLogError, message);
}

}  // namespace server
}  // namespace square_dance
