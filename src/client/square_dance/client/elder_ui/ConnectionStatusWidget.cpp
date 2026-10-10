#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"

#include "square_dance/client/elder_ui/AppTheme.h"

namespace square_dance {
namespace client {

ConnectionStatusWidget::ConnectionStatusWidget(QWidget* parent)
    : QLabel(parent) {
  setFont(AppTheme::CaptionFont());
  setMinimumHeight(28);
  setAlignment(Qt::AlignCenter);
  setConnected(false);
}

void ConnectionStatusWidget::setConnected(bool connected) {
  if (connected) {
    setText(QString::fromUtf8("Live"));
    setStyleSheet(QString::fromUtf8(
        "QLabel {"
        "  color: %1; background: #ecfdf5;"
        "  border: 1px solid %1; border-radius: 14px;"
        "  padding: 4px 12px; font-weight: 600;"
        "}")
        .arg(AppTheme::Success()));
  } else {
    setText(QString::fromUtf8("Offline"));
    setStyleSheet(QString::fromUtf8(
        "QLabel {"
        "  color: %1; background: #fef2f2;"
        "  border: 1px solid %1; border-radius: 14px;"
        "  padding: 4px 12px; font-weight: 600;"
        "}")
        .arg(AppTheme::Danger()));
  }
}

}  // namespace client
}  // namespace square_dance
