#include "square_dance/server/ServerApp.h"
#include "square_dance/server/ServerConfig.h"
#include "square_dance/server/log.h"

#include <iostream>
#include <string>

namespace {

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " --config <path-to-server.json>\n"
            << "  Serves HTTP search API and WebSocket push; runs ingest and "
               "URL health timers.\n"
            << "  Example (from repo root):\n"
            << "    SquareDanceServer.exe --config config/server.json.example\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_path;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    } else {
      square_dance::server::LogError("unknown argument: " + arg);
      PrintUsage(argv[0]);
      return 2;
    }
  }

  if (config_path.empty()) {
    PrintUsage(argv[0]);
    return 2;
  }

  square_dance::server::ServerConfig config;
  std::string error;
  if (!square_dance::server::LoadServerConfigFromFile(config_path, &config,
                                                     &error)) {
    square_dance::server::LogError("failed to load config: " + error);
    return 1;
  }

  square_dance::server::ServerApp app(config);
  return app.RunBlocking();
}
