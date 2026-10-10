#include "square_dance/client/elder_ui/PlayerPane.h"

#include "square_dance/client/elder_ui/ElderStyle.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

PlayerPane::PlayerPane(QWidget* parent)
    : QWidget(parent), player_(new VideoPlayerWidget(this)) {
  QPushButton* play = new QPushButton(QString::fromUtf8("播放"), this);
  QPushButton* pause = new QPushButton(QString::fromUtf8("暂停"), this);
  QPushButton* stop = new QPushButton(QString::fromUtf8("停止"), this);
  QPushButton* download = new QPushButton(QString::fromUtf8("下载"), this);

  play->setFont(ElderStyle::ButtonFont());
  pause->setFont(ElderStyle::ButtonFont());
  stop->setFont(ElderStyle::ButtonFont());
  download->setFont(ElderStyle::ButtonFont());
  play->setStyleSheet(ElderStyle::PlayButtonStyle());
  pause->setStyleSheet(ElderStyle::SecondaryButtonStyle());
  stop->setStyleSheet(ElderStyle::SecondaryButtonStyle());
  download->setStyleSheet(ElderStyle::PrimaryButtonStyle());
  play->setCursor(Qt::PointingHandCursor);
  pause->setCursor(Qt::PointingHandCursor);
  stop->setCursor(Qt::PointingHandCursor);
  download->setCursor(Qt::PointingHandCursor);

  // Do NOT put a stylesheet on any ancestor of QVideoWidget — it blanks video.
  QFrame* chrome = new QFrame(this);
  chrome->setFrameShape(QFrame::Box);
  chrome->setLineWidth(3);
  chrome->setMidLineWidth(0);
  QPalette chrome_pal = chrome->palette();
  chrome_pal.setColor(QPalette::Window, QColor(16, 24, 20));
  chrome_pal.setColor(QPalette::Dark, QColor(47, 111, 78));
  chrome_pal.setColor(QPalette::Light, QColor(47, 111, 78));
  chrome_pal.setColor(QPalette::Shadow, QColor(47, 111, 78));
  chrome->setPalette(chrome_pal);
  chrome->setAutoFillBackground(true);

  QVBoxLayout* chrome_layout = new QVBoxLayout(chrome);
  chrome_layout->setContentsMargins(6, 6, 6, 6);
  chrome_layout->addWidget(player_);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->setSpacing(12);
  buttons->addWidget(play, 2);
  buttons->addWidget(pause, 1);
  buttons->addWidget(stop, 1);
  buttons->addWidget(download, 1);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);
  layout->addWidget(chrome, 1);
  layout->addLayout(buttons);

  connect(play, &QPushButton::clicked, this, &PlayerPane::playClicked);
  connect(pause, &QPushButton::clicked, this, &PlayerPane::pauseClicked);
  connect(stop, &QPushButton::clicked, this, &PlayerPane::stopClicked);
  connect(download, &QPushButton::clicked, this, &PlayerPane::downloadClicked);
}

VideoPlayerWidget* PlayerPane::playerWidget() const { return player_; }

}  // namespace client
}  // namespace square_dance
