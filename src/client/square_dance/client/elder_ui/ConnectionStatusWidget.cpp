#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"

#include <QFont>

namespace square_dance {
namespace client {

ConnectionStatusWidget::ConnectionStatusWidget(QWidget* parent)
    : QLabel(parent) {
  QFont font = this->font();
  font.setPointSize(16);
  setFont(font);
  setConnected(false);
}

void ConnectionStatusWidget::setConnected(bool connected) {
  if (connected) {
    setText(QString::fromUtf8("已连接"));
    setStyleSheet(QString::fromUtf8("color: #0a7a2f;"));
  } else {
    setText(QString::fromUtf8("未连接"));
    setStyleSheet(QString::fromUtf8("color: #a33;"));
  }
}

}  // namespace client
}  // namespace square_dance
