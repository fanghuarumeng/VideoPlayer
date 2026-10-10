#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"

#include "square_dance/client/elder_ui/ElderStyle.h"

namespace square_dance {
namespace client {

ConnectionStatusWidget::ConnectionStatusWidget(QWidget* parent)
    : QLabel(parent) {
  setFont(ElderStyle::BodyFont());
  setMinimumHeight(40);
  setAlignment(Qt::AlignCenter);
  setConnected(false);
}

void ConnectionStatusWidget::setConnected(bool connected) {
  if (connected) {
    setText(QString::fromUtf8("● 已连接"));
    setStyleSheet(QString::fromUtf8(
                      "QLabel {"
                      "  color: %1;"
                      "  background: #e5f6ea;"
                      "  border: 2px solid %1;"
                      "  border-radius: 20px;"
                      "  padding: 6px 16px;"
                      "}")
                      .arg(ElderStyle::Connected()));
  } else {
    setText(QString::fromUtf8("○ 未连接"));
    setStyleSheet(QString::fromUtf8(
                      "QLabel {"
                      "  color: %1;"
                      "  background: #fdecea;"
                      "  border: 2px solid %1;"
                      "  border-radius: 20px;"
                      "  padding: 6px 16px;"
                      "}")
                      .arg(ElderStyle::DangerSoft()));
  }
}

}  // namespace client
}  // namespace square_dance
