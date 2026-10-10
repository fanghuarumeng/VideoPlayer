#pragma once

#include <QFile>
#include <QFont>
#include <QString>
#include <QTextStream>

namespace square_dance {
namespace client {
namespace AppTheme {

// Design tokens aligned with dark_player.qss
inline QString WindowBg() { return QString::fromUtf8("#353A40"); }
inline QString Surface() { return QString::fromUtf8("#2E3338"); }
inline QString SurfaceRaised() { return QString::fromUtf8("#3A4048"); }
inline QString Border() { return QString::fromUtf8("rgba(255,255,255,0.15)"); }
inline QString TextPrimary() { return QString::fromUtf8("#FFFFFF"); }
inline QString TextSecondary() { return QString::fromUtf8("#B0B3B8"); }
inline QString Accent() { return QString::fromUtf8("#5086FF"); }
inline QString AccentHover() { return QString::fromUtf8("#6A9AFF"); }
inline QString AccentPressed() { return QString::fromUtf8("#3F6FE0"); }
inline QString Success() { return QString::fromUtf8("#22c55e"); }
inline QString Danger() { return QString::fromUtf8("#ef4444"); }
inline QString ListHover() { return QString::fromUtf8("rgba(255,255,255,0.08)"); }
inline QString PlayerBg() { return QString::fromUtf8("#1E2226"); }
inline QString BufferFill() { return QString::fromUtf8("#D8D0C0"); }
inline QString TrackFill() {
  return QString::fromUtf8("rgba(255,255,255,0.20)");
}

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
  f.setPointSize(14);
  f.setBold(true);
  return f;
}

inline QFont CaptionFont() {
  QFont f = UiFont();
  f.setPointSize(9);
  return f;
}

inline QString GlobalStylesheet() {
  QFile file(QString::fromUtf8(":/app/styles/dark_player.qss"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return QString();
  }
  QTextStream in(&file);
  in.setCodec("UTF-8");
  return in.readAll();
}

// Kept for call sites that still set local styles; prefer objectName + QSS.
inline QString IconButtonStyle() { return QString(); }
inline QString GhostButtonStyle() { return QString(); }
inline QString NavButtonStyle() { return QString(); }
inline QString PrimaryButtonStyle() { return QString(); }
inline QString SearchFieldStyle() { return QString(); }
inline QString ListPanelStyle() { return QString(); }
inline QString PanelTitleStyle() {
  return QString::fromUtf8(
      "QLabel { color: #B0B3B8; background: transparent; }");
}
inline QString BrandTitleStyle() {
  return QString::fromUtf8(
      "QLabel { color: #FFFFFF; background: transparent; }");
}
inline QString VideoChromeStyle() { return QString(); }
inline QString ProgressSliderStyle() { return QString(); }
inline QString MoreButtonStyle() { return QString(); }
inline QString GlassPanelStyle() { return QString(); }
inline QString GlassSidebarStyle() { return QString(); }

}  // namespace AppTheme
}  // namespace client
}  // namespace square_dance
