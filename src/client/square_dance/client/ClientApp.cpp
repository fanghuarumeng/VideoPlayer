#include "square_dance/client/ClientApp.h"

#include "square_dance/client/qml/PlayerUiBridge.h"
#include "square_dance/client/qml/VideoFrameItem.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

// Force-link qrc from the static client lib (MSVC may drop unused init).
inline void InitClientResources() { Q_INIT_RESOURCE(app); }

namespace square_dance {
namespace client {

ClientApp::ClientApp(int& argc, char** argv, const ClientConfig& config)
    : config_(config) {
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  app_.reset(new QApplication(argc, argv));
  InitClientResources();
  QQuickStyle::setStyle(QString::fromUtf8("Default"));

  qmlRegisterType<VideoFrameItem>("SquareDance", 1, 0, "VideoFrameItem");

  bridge_.reset(new PlayerUiBridge(config_));
  engine_.reset(new QQmlApplicationEngine());
  const QString app_dir = QCoreApplication::applicationDirPath();
  engine_->addImportPath(app_dir);
  engine_->addImportPath(app_dir + QString::fromUtf8("/qml"));
  engine_->rootContext()->setContextProperty(QString::fromUtf8("playerBridge"),
                                             bridge_.get());
  engine_->load(QUrl(QString::fromUtf8("qrc:/app/qml/Main.qml")));
  if (engine_->rootObjects().isEmpty()) {
    // Fallback: load from source tree during development.
    engine_->load(QUrl::fromLocalFile(
        QString::fromUtf8("f:/git/aicode/src/client/resources/qml/Main.qml")));
  }
}

ClientApp::~ClientApp() {}

int ClientApp::Run() {
  if (engine_->rootObjects().isEmpty()) {
    return 1;
  }
  return app_->exec();
}

const ClientConfig& ClientApp::config() const { return config_; }

}  // namespace client
}  // namespace square_dance
