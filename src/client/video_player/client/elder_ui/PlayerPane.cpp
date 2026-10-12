#include "video_player/client/elder_ui/PlayerPane.h"

#include "video_player/client/elder_ui/AppIcons.h"
#include "video_player/client/elder_ui/BufferedSeekSlider.h"
#include "video_player/client/playback/VideoPlayerWidget.h"

#include <cmath>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace video_player {
namespace client {

namespace {

void StyleControlButton(QPushButton* btn, const QIcon& icon,
                        const QString& tip) {
  btn->setObjectName(QString::fromUtf8("controlButton"));
  btn->setIcon(icon);
  btn->setIconSize(QSize(20, 20));
  btn->setToolTip(tip);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setFlat(true);
}

}  // namespace

PlayerPane::PlayerPane(QWidget* parent)
    : QWidget(parent),
      player_(new VideoPlayerWidget(this)),
      title_label_(new QLabel(QString::fromUtf8("选择视频开始播放"), this)),
      time_label_(new QLabel(QString::fromUtf8("0:00 / 0:00"), this)),
      progress_slider_(new BufferedSeekSlider(Qt::Horizontal, this)),
      play_btn_(new QPushButton(this)),
      pause_btn_(new QPushButton(this)),
      stop_btn_(new QPushButton(this)),
      download_btn_(new QPushButton(this)),
      duration_sec_(0.0),
      position_sec_(0.0),
      scrubbing_(false) {
  title_label_->setObjectName(QString::fromUtf8("videoTitleLabel"));
  title_label_->setWordWrap(true);
  time_label_->setObjectName(QString::fromUtf8("timestampLabel"));

  StyleControlButton(play_btn_, AppIcons::Play(), QString::fromUtf8("播放"));
  StyleControlButton(pause_btn_, AppIcons::Pause(), QString::fromUtf8("暂停"));
  StyleControlButton(stop_btn_, AppIcons::Stop(), QString::fromUtf8("停止"));
  StyleControlButton(download_btn_, AppIcons::Download(),
                     QString::fromUtf8("下载"));

  QFrame* chrome = new QFrame(this);
  chrome->setObjectName(QString::fromUtf8("videoChrome"));
  chrome->setAttribute(Qt::WA_StyledBackground, true);
  QVBoxLayout* chrome_layout = new QVBoxLayout(chrome);
  chrome_layout->setContentsMargins(0, 0, 0, 0);
  chrome_layout->addWidget(player_);

  QFrame* meta_panel = new QFrame(this);
  meta_panel->setObjectName(QString::fromUtf8("metaPanel"));
  meta_panel->setAttribute(Qt::WA_StyledBackground, true);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->setContentsMargins(0, 4, 0, 0);
  buttons->setSpacing(4);
  buttons->addWidget(play_btn_);
  buttons->addWidget(pause_btn_);
  buttons->addWidget(stop_btn_);
  buttons->addStretch(1);
  buttons->addWidget(download_btn_);

  QVBoxLayout* meta = new QVBoxLayout(meta_panel);
  meta->setContentsMargins(4, 12, 4, 4);
  meta->setSpacing(8);
  meta->addWidget(title_label_);
  meta->addWidget(time_label_, 0, Qt::AlignLeft);
  meta->addWidget(progress_slider_);
  meta->addLayout(buttons);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 12, 0);
  layout->setSpacing(8);
  layout->addWidget(chrome, 1);
  layout->addWidget(meta_panel, 0);

  connect(play_btn_, &QPushButton::clicked, this, &PlayerPane::playClicked);
  connect(pause_btn_, &QPushButton::clicked, this, &PlayerPane::pauseClicked);
  connect(stop_btn_, &QPushButton::clicked, this, &PlayerPane::stopClicked);
  connect(download_btn_, &QPushButton::clicked, this,
          &PlayerPane::downloadClicked);
  connect(progress_slider_, &BufferedSeekSlider::seekCommitted, this,
          &PlayerPane::onSeekCommitted);
  connect(progress_slider_, &QSlider::sliderPressed, this, [this]() {
    scrubbing_ = true;
    emit scrubbingChanged(true);
  });
  connect(progress_slider_, &QSlider::sliderReleased, this, [this]() {
    scrubbing_ = false;
    emit scrubbingChanged(false);
  });
}

VideoPlayerWidget* PlayerPane::playerWidget() const { return player_; }

QString PlayerPane::FormatTime(double seconds) {
  if (seconds < 0.0 || !std::isfinite(seconds)) {
    seconds = 0.0;
  }
  const int total = static_cast<int>(seconds + 0.5);
  const int m = total / 60;
  const int s = total % 60;
  return QString::fromUtf8("%1:%2")
      .arg(m)
      .arg(s, 2, 10, QChar('0'));
}

void PlayerPane::UpdateTimeLabel(double position_sec, double duration_sec) {
  const QString dur =
      duration_sec > 0.0 ? FormatTime(duration_sec) : QString::fromUtf8("--:--");
  time_label_->setText(FormatTime(position_sec) + QString::fromUtf8(" / ") +
                       dur);
}

void PlayerPane::setTitle(const QString& title) {
  if (title.trimmed().isEmpty()) {
    title_label_->setText(QString::fromUtf8("选择视频开始播放"));
  } else {
    title_label_->setText(title);
  }
}

void PlayerPane::setPlaybackProgress(double position_sec, double duration_sec) {
  duration_sec_ = duration_sec;
  position_sec_ = position_sec;
  UpdateTimeLabel(position_sec, duration_sec);
  if (duration_sec > 0.0) {
    if (!scrubbing_) {
      const int slider_val =
          static_cast<int>((position_sec / duration_sec) * 1000.0 + 0.5);
      progress_slider_->setValue(qBound(0, slider_val, 1000));
    }
  } else if (!scrubbing_) {
    progress_slider_->setValue(0);
  }
}

void PlayerPane::setBufferProgress(double buffered_ratio) {
  progress_slider_->setBufferRatio(buffered_ratio);
}

void PlayerPane::resetPlaybackProgress() {
  duration_sec_ = 0.0;
  position_sec_ = 0.0;
  scrubbing_ = false;
  progress_slider_->setValue(0);
  progress_slider_->setBufferRatio(0.0);
  UpdateTimeLabel(0.0, 0.0);
  setPausedState(false);
}

void PlayerPane::setPausedState(bool paused) {
  if (paused) {
    pause_btn_->setToolTip(QString::fromUtf8("恢复"));
    pause_btn_->setIcon(AppIcons::Play());
  } else {
    pause_btn_->setToolTip(QString::fromUtf8("暂停"));
    pause_btn_->setIcon(AppIcons::Pause());
  }
}

void PlayerPane::onSeekCommitted(double ratio) {
  if (duration_sec_ <= 0.0) {
    return;
  }
  const double pos = qBound(0.0, ratio, 1.0) * duration_sec_;
  position_sec_ = pos;
  UpdateTimeLabel(pos, duration_sec_);
  emit seekRequested(pos);
}

}  // namespace client
}  // namespace video_player
