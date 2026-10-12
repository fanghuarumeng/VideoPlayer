#pragma once

#include "video_player/client/ClientConfig.h"

#include <memory>

class QApplication;
class QQmlApplicationEngine;

namespace video_player {
namespace client {

class PlayerUiBridge;

class ClientApp {
 public:
  ClientApp(int& argc, char** argv, const ClientConfig& config);
  ~ClientApp();

  int Run();

  const ClientConfig& config() const;

 private:
  ClientApp(const ClientApp&);
  ClientApp& operator=(const ClientApp&);

  ClientConfig config_;
  std::unique_ptr<QApplication> app_;
  std::unique_ptr<QQmlApplicationEngine> engine_;
  std::unique_ptr<PlayerUiBridge> bridge_;
};

}  // namespace client
}  // namespace video_player
