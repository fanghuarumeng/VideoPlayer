#include "square_dance/client/elder_ui/PlayerPane.h"

#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QFont>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

PlayerPane::PlayerPane(QWidget* parent)
    : QWidget(parent), player_(new VideoPlayerWidget(this)) {
  QFont title;
  title.setPointSize(18);

  QPushButton* play = new QPushButton(QString::fromUtf8("播放"), this);
  QPushButton* pause = new QPushButton(QString::fromUtf8("暂停"), this);
  QPushButton* stop = new QPushButton(QString::fromUtf8("停止"), this);
  QPushButton* download = new QPushButton(QString::fromUtf8("下载"), this);
  play->setFont(title);
  pause->setFont(title);
  stop->setFont(title);
  download->setFont(title);
  play->setMinimumHeight(48);
  pause->setMinimumHeight(48);
  stop->setMinimumHeight(48);
  download->setMinimumHeight(48);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->addWidget(play);
  buttons->addWidget(pause);
  buttons->addWidget(stop);
  buttons->addWidget(download);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(player_, 1);
  layout->addLayout(buttons);

  connect(play, &QPushButton::clicked, this, &PlayerPane::playClicked);
  connect(pause, &QPushButton::clicked, this, &PlayerPane::pauseClicked);
  connect(stop, &QPushButton::clicked, this, &PlayerPane::stopClicked);
  connect(download, &QPushButton::clicked, this, &PlayerPane::downloadClicked);
}

VideoPlayerWidget* PlayerPane::playerWidget() const { return player_; }

}  // namespace client
}  // namespace square_dance
