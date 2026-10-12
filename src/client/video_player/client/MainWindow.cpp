#include "video_player/client/MainWindow.h"

#include <QLabel>
#include <QWidget>

namespace video_player {
namespace client {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QString::fromUtf8("Video Player"));
  resize(960, 640);

  QLabel* placeholder = new QLabel(QString::fromUtf8("Video Player"), this);
  placeholder->setAlignment(Qt::AlignCenter);
  setCentralWidget(placeholder);
}

}  // namespace client
}  // namespace video_player
