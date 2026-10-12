#pragma once

#include <QLabel>

namespace video_player {
namespace client {

class ConnectionStatusWidget : public QLabel {
  Q_OBJECT

 public:
  explicit ConnectionStatusWidget(QWidget* parent = 0);

 public slots:
  void setConnected(bool connected);
};

}  // namespace client
}  // namespace video_player
