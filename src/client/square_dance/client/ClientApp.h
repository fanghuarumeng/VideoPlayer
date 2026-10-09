#pragma once

#include "square_dance/client/ClientConfig.h"

#include <memory>

class QApplication;

namespace square_dance {
namespace client {

class MainWindow;

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
  std::unique_ptr<MainWindow> window_;
};

}  // namespace client
}  // namespace square_dance
