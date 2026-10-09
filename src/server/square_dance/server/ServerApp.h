#pragma once

#include "square_dance/server/ServerConfig.h"

namespace square_dance {
namespace server {

// Shell-only for Task 2: holds config; later tasks wire store/ingest/api.
class ServerApp {
 public:
  explicit ServerApp(const ServerConfig& config);

  const ServerConfig& config() const;

  // Returns 0 on success. Task 2: validate + log and return (no listen yet).
  int RunShellOnce();

 private:
  ServerConfig config_;
};

}  // namespace server
}  // namespace square_dance
