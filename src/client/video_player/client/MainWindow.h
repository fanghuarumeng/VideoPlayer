#pragma once

#include <QMainWindow>

namespace video_player {
namespace client {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = 0);
};

}  // namespace client
}  // namespace video_player
