#pragma once

#include "square_dance/client/ClientConfig.h"

#include <memory>

class QApplication;

namespace square_dance {
namespace client {

class ElderMainWindow;

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
  std::unique_ptr<ElderMainWindow> window_;
};

}  // namespace client
}  // namespace square_dance
