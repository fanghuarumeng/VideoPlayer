#include "video_player/client/ClientApp.h"
#include "video_player/client/ClientConfig.h"

#include <QString>

#include <iostream>
#include <string>

namespace {

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " --config <path-to-client.json>\n"
            << "  Example (from repo root):\n"
            << "    VideoPlayerApp.exe --config config/client.json.example\n";
}

}  // namespace

int main(int argc, char** argv) {
  QString config_path;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = QString::fromLocal8Bit(argv[++i]);
    } else if (arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    } else {
      std::cerr << "unknown argument: " << arg << "\n";
      PrintUsage(argv[0]);
      return 2;
    }
  }

  if (config_path.isEmpty()) {
    PrintUsage(argv[0]);
    return 2;
  }

  video_player::client::ClientConfig config;
  QString error;
  if (!video_player::client::LoadClientConfigFromFile(config_path, &config,
                                                      &error)) {
    std::cerr << "failed to load config: " << error.toLocal8Bit().constData()
              << "\n";
    return 1;
  }

  video_player::client::ClientApp app(argc, argv, config);
  return app.Run();
}
