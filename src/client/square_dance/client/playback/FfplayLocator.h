#pragma once

#include <QString>

namespace square_dance {
namespace client {

// Resolves ffplay.exe / ffmpeg.exe: env override, third_party/ffmpeg/bin next to
// the app or repo, then PATH.
QString FindFfplayExecutable();
QString FindFfmpegExecutable();

}  // namespace client
}  // namespace square_dance
