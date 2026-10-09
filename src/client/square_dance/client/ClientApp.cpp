#include "square_dance/client/ClientApp.h"

#include "square_dance/client/MainWindow.h"

#include <QApplication>

namespace square_dance {
namespace client {

ClientApp::ClientApp(int& argc, char** argv, const ClientConfig& config)
    : config_(config),
      app_(new QApplication(argc, argv)),
      window_(new MainWindow()) {}

ClientApp::~ClientApp() {}

int ClientApp::Run() {
  window_->show();
  return app_->exec();
}

const ClientConfig& ClientApp::config() const { return config_; }

}  // namespace client
}  // namespace square_dance
