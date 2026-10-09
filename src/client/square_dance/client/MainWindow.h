#pragma once

#include <QMainWindow>

namespace square_dance {
namespace client {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = 0);
};

}  // namespace client
}  // namespace square_dance
