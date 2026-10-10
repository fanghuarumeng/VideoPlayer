#pragma once

#include <QFont>
#include <QString>

namespace square_dance {
namespace client {
namespace AppTheme {

// Light media-app palette (clean white + sky accent).
inline QString WindowBg() { return QString::fromUtf8("#f1f5f9"); }
inline QString Surface() { return QString::fromUtf8("#ffffff"); }
inline QString SurfaceRaised() { return QString::fromUtf8("#e2e8f0"); }
inline QString Border() { return QString::fromUtf8("#cbd5e1"); }
inline QString TextPrimary() { return QString::fromUtf8("#0f172a"); }
inline QString TextSecondary() { return QString::fromUtf8("#64748b"); }
inline QString Accent() { return QString::fromUtf8("#0284c7"); }
inline QString AccentHover() { return QString::fromUtf8("#0ea5e9"); }
inline QString AccentPressed() { return QString::fromUtf8("#0369a1"); }
inline QString Success() { return QString::fromUtf8("#16a34a"); }
inline QString Danger() { return QString::fromUtf8("#dc2626"); }
inline QString ListHover() { return QString::fromUtf8("#f1f5f9"); }
inline QString PlayerBg() { return QString::fromUtf8("#0f172a"); }

inline QFont UiFont() {
  QFont f(QString::fromUtf8("Segoe UI"));
  if (!f.exactMatch()) {
    f = QFont(QString::fromUtf8("Microsoft YaHei UI"));
  }
  f.setPointSize(10);
  return f;
}

inline QFont TitleFont() {
  QFont f = UiFont();
  f.setPointSize(13);
  f.setBold(true);
  return f;
}

inline QFont CaptionFont() {
  QFont f = UiFont();
  f.setPointSize(9);
  return f;
}

inline QString GlobalStylesheet() {
  return QString::fromUtf8(
      "QMainWindow, QWidget { color: %1; }"
      "QSplitter::handle { background: %2; width: 1px; }"
      "QScrollBar:vertical { background: %3; width: 10px; margin: 0; }"
      "QScrollBar::handle:vertical { background: %4; min-height: 24px; border-radius: 5px; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
      "QStatusBar { background: %3; color: %5; border-top: 1px solid %2; }")
      .arg(TextPrimary(), Border(), Surface(), SurfaceRaised(), TextSecondary());
}

inline QString IconButtonStyle() {
  return QString::fromUtf8(
      "QPushButton {"
      "  background: %1; color: %2; border: 1px solid %3;"
      "  border-radius: 8px; padding: 8px 14px; min-height: 36px;"
      "}"
      "QPushButton:hover { background: %4; border-color: %5; }"
      "QPushButton:pressed { background: %6; }"
      "QPushButton:disabled { color: %7; background: %1; }")
      .arg(Surface(), TextPrimary(), Border(), ListHover(), Accent(),
           SurfaceRaised(), TextSecondary());
}

inline QString PrimaryButtonStyle() {
  return QString::fromUtf8(
      "QPushButton {"
      "  background: %1; color: white; border: none;"
      "  border-radius: 8px; padding: 8px 16px; min-height: 36px;"
      "  font-weight: 600;"
      "}"
      "QPushButton:hover { background: %2; }"
      "QPushButton:pressed { background: %3; }")
      .arg(Accent(), AccentHover(), AccentPressed());
}

inline QString SearchFieldStyle() {
  return QString::fromUtf8(
      "QLineEdit {"
      "  background: %1; color: %2; border: 1px solid %3;"
      "  border-radius: 8px; padding: 8px 12px; min-height: 20px;"
      "  selection-background-color: %4;"
      "}"
      "QLineEdit:focus { border-color: %4; }")
      .arg(Surface(), TextPrimary(), Border(), Accent());
}

inline QString ListPanelStyle() {
  return QString::fromUtf8(
      "QListView {"
      "  background: %1; border: 1px solid %2; border-radius: 10px;"
      "  padding: 4px; outline: none; color: %3;"
      "}"
      "QListView::item { border-radius: 6px; padding: 4px; margin: 1px 0; }"
      "QListView::item:selected { background: %4; color: white; }"
      "QListView::item:hover:!selected { background: %5; }")
      .arg(Surface(), Border(), TextPrimary(), Accent(), ListHover());
}

inline QString PanelTitleStyle() {
  return QString::fromUtf8("QLabel { color: %1; background: transparent; }")
      .arg(TextSecondary());
}

inline QString BrandTitleStyle() {
  return QString::fromUtf8("QLabel { color: %1; background: transparent; }")
      .arg(TextPrimary());
}

inline QString VideoChromeStyle() {
  return QString::fromUtf8(
      "QFrame#videoChrome {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: 12px;"
      "}")
      .arg(PlayerBg(), Border());
}

inline QString ProgressSliderStyle() {
  return QString::fromUtf8(
      "QSlider::groove:horizontal {"
      "  height: 6px; background: %1; border-radius: 3px;"
      "}"
      "QSlider::sub-page:horizontal {"
      "  background: %2; border-radius: 3px;"
      "}"
      "QSlider::add-page:horizontal {"
      "  background: %1; border-radius: 3px;"
      "}"
      "QSlider::handle:horizontal {"
      "  width: 14px; margin: -5px 0; background: %2;"
      "  border: 2px solid %3; border-radius: 7px;"
      "}"
      "QSlider::handle:horizontal:hover { background: %4; }")
      .arg(SurfaceRaised(), Accent(), Border(), AccentHover());
}

}  // namespace AppTheme
}  // namespace client
}  // namespace square_dance
