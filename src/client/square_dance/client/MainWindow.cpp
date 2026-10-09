#include "square_dance/client/MainWindow.h"

#include <QLabel>
#include <QWidget>

namespace square_dance {
namespace client {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QString::fromUtf8("广场舞"));
  resize(960, 640);

  QLabel* placeholder = new QLabel(QString::fromUtf8("广场舞客户端"), this);
  placeholder->setAlignment(Qt::AlignCenter);
  setCentralWidget(placeholder);
}

}  // namespace client
}  // namespace square_dance
