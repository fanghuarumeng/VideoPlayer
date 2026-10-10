#pragma once

#include <QFont>
#include <QString>

namespace square_dance {
namespace client {
namespace ElderStyle {

// Morning-park palette: soft sage field, ink text, coral play CTA.
// Avoid purple gradients / cream-serif-terracotta clusters.
inline QString WindowBackground() {
  return QString::fromUtf8("#e7f0ea");
}

inline QString PanelBackground() {
  return QString::fromUtf8("#f7fbf8");
}

inline QString Ink() { return QString::fromUtf8("#1c2b24"); }

inline QString Muted() { return QString::fromUtf8("#4d6358"); }

inline QString Accent() { return QString::fromUtf8("#2f6f4e"); }

inline QString AccentDeep() { return QString::fromUtf8("#245a3e"); }

inline QString PlayColor() { return QString::fromUtf8("#d35400"); }

inline QString PlayHover() { return QString::fromUtf8("#e67e22"); }

inline QString DangerSoft() { return QString::fromUtf8("#a94442"); }

inline QString Connected() { return QString::fromUtf8("#1e7a45"); }

inline QFont TitleFont() {
  QFont f(QString::fromUtf8("Microsoft YaHei UI"));
  f.setPointSize(22);
  f.setBold(true);
  return f;
}

inline QFont BodyFont() {
  QFont f(QString::fromUtf8("Microsoft YaHei UI"));
  f.setPointSize(16);
  return f;
}

inline QFont ButtonFont() {
  QFont f(QString::fromUtf8("Microsoft YaHei UI"));
  f.setPointSize(18);
  f.setBold(true);
  return f;
}

inline QString PrimaryButtonStyle() {
  return QString::fromUtf8(
      "QPushButton {"
      "  background-color: %1;"
      "  color: white;"
      "  border: none;"
      "  border-radius: 10px;"
      "  padding: 10px 18px;"
      "  min-height: 52px;"
      "}"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: %3; }")
      .arg(Accent(), AccentDeep(), QString::fromUtf8("#1c4630"));
}

inline QString PlayButtonStyle() {
  return QString::fromUtf8(
      "QPushButton {"
      "  background-color: %1;"
      "  color: white;"
      "  border: none;"
      "  border-radius: 10px;"
      "  padding: 10px 18px;"
      "  min-height: 52px;"
      "}"
      "QPushButton:hover { background-color: %2; }"
      "QPushButton:pressed { background-color: #b84a00; }")
      .arg(PlayColor(), PlayHover());
}

inline QString SecondaryButtonStyle() {
  return QString::fromUtf8(
      "QPushButton {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 2px solid #c5d6cc;"
      "  border-radius: 10px;"
      "  padding: 10px 18px;"
      "  min-height: 52px;"
      "}"
      "QPushButton:hover { background-color: #eef6f1; border-color: %3; }")
      .arg(PanelBackground(), Ink(), Accent());
}

inline QString SearchEditStyle() {
  return QString::fromUtf8(
      "QLineEdit {"
      "  background: white;"
      "  color: %1;"
      "  border: 2px solid #c5d6cc;"
      "  border-radius: 12px;"
      "  padding: 10px 14px;"
      "  min-height: 48px;"
      "  selection-background-color: %2;"
      "}"
      "QLineEdit:focus { border-color: %2; }")
      .arg(Ink(), Accent());
}

inline QString ListStyle() {
  return QString::fromUtf8(
      "QListView {"
      "  background: %1;"
      "  border: 2px solid #c5d6cc;"
      "  border-radius: 14px;"
      "  padding: 6px;"
      "  outline: none;"
      "  color: %2;"
      "}"
      "QListView::item {"
      "  min-height: 52px;"
      "  padding: 10px 12px;"
      "  border-radius: 10px;"
      "  margin: 3px;"
      "}"
      "QListView::item:selected {"
      "  background: %3;"
      "  color: white;"
      "}"
      "QListView::item:hover:!selected { background: #e4f0e9; }")
      .arg(PanelBackground(), Ink(), Accent());
}

inline QString StatusStyle() {
  return QString::fromUtf8(
      "QLabel {"
      "  background: %1;"
      "  color: %2;"
      "  border: 1px solid #c5d6cc;"
      "  border-radius: 10px;"
      "  padding: 10px 14px;"
      "}")
      .arg(PanelBackground(), Muted());
}

inline QString BrandStyle() {
  return QString::fromUtf8("QLabel { color: %1; background: transparent; }")
      .arg(AccentDeep());
}

inline QString VideoChromeStyle() {
  // Applied only to a sibling frame around the view, not an ancestor stylesheet
  // that would paint over a native video surface.
  return QString::fromUtf8(
      "QFrame#videoChrome {"
      "  background-color: #101814;"
      "  border: 3px solid #2f6f4e;"
      "  border-radius: 14px;"
      "}");
}

}  // namespace ElderStyle
}  // namespace client
}  // namespace square_dance
