#include "square_dance/client/elder_ui/PlayerPane.h"

#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <cmath>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

PlayerPane::PlayerPane(QWidget* parent)
    : QWidget(parent),
      player_(new VideoPlayerWidget(this)),
      progress_slider_(new QSlider(Qt::Horizontal, this)),
      time_current_(new QLabel(QString::fromUtf8("0:00"), this)),
      time_total_(new QLabel(QString::fromUtf8("0:00"), this)),
      duration_sec_(0.0),
      scrubbing_(false) {
  QPushButton* play = new QPushButton(QString::fromUtf8("播放"), this);
  QPushButton* pause = new QPushButton(QString::fromUtf8("暂停"), this);
  QPushButton* stop = new QPushButton(QString::fromUtf8("停止"), this);
  QPushButton* download = new QPushButton(QString::fromUtf8("下载"), this);

  play->setIcon(AppIcons::Play());
  pause->setIcon(AppIcons::Pause());
  stop->setIcon(AppIcons::Stop());
  download->setIcon(AppIcons::Download());

  const QSize icon_size(20, 20);
  play->setIconSize(icon_size);
  pause->setIconSize(icon_size);
  stop->setIconSize(icon_size);
  download->setIconSize(icon_size);

  play->setFont(AppTheme::UiFont());
  pause->setFont(AppTheme::UiFont());
  stop->setFont(AppTheme::UiFont());
  download->setFont(AppTheme::UiFont());

  play->setStyleSheet(AppTheme::PrimaryButtonStyle());
  pause->setStyleSheet(AppTheme::IconButtonStyle());
  stop->setStyleSheet(AppTheme::IconButtonStyle());
  download->setStyleSheet(AppTheme::IconButtonStyle());

  play->setCursor(Qt::PointingHandCursor);
  pause->setCursor(Qt::PointingHandCursor);
  stop->setCursor(Qt::PointingHandCursor);
  download->setCursor(Qt::PointingHandCursor);

  progress_slider_->setRange(0, 1000);
  progress_slider_->setValue(0);
  progress_slider_->setStyleSheet(AppTheme::ProgressSliderStyle());
  progress_slider_->setCursor(Qt::PointingHandCursor);

  time_current_->setFont(AppTheme::CaptionFont());
  time_total_->setFont(AppTheme::CaptionFont());
  time_current_->setStyleSheet(AppTheme::PanelTitleStyle());
  time_total_->setStyleSheet(AppTheme::PanelTitleStyle());
  time_current_->setMinimumWidth(40);
  time_total_->setMinimumWidth(40);
  time_total_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QFrame* chrome = new QFrame(this);
  chrome->setObjectName(QString::fromUtf8("videoChrome"));
  chrome->setStyleSheet(AppTheme::VideoChromeStyle());

  QVBoxLayout* chrome_layout = new QVBoxLayout(chrome);
  chrome_layout->setContentsMargins(0, 0, 0, 0);
  chrome_layout->addWidget(player_);

  QWidget* transport = new QWidget(this);
  transport->setStyleSheet(QString::fromUtf8(
      "QWidget { background: %1; border: 1px solid %2; border-radius: 10px; }")
                               .arg(AppTheme::Surface(), AppTheme::Border()));

  QHBoxLayout* progress_row = new QHBoxLayout();
  progress_row->setSpacing(8);
  progress_row->addWidget(time_current_);
  progress_row->addWidget(progress_slider_, 1);
  progress_row->addWidget(time_total_);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->setContentsMargins(12, 0, 12, 10);
  buttons->setSpacing(10);
  buttons->addWidget(play);
  buttons->addWidget(pause);
  buttons->addWidget(stop);
  buttons->addStretch(1);
  buttons->addWidget(download);

  QVBoxLayout* transport_layout = new QVBoxLayout(transport);
  transport_layout->setContentsMargins(12, 10, 12, 0);
  transport_layout->setSpacing(8);
  transport_layout->addLayout(progress_row);
  transport_layout->addLayout(buttons);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);
  layout->addWidget(chrome, 1);
  layout->addWidget(transport);

  connect(play, &QPushButton::clicked, this, &PlayerPane::playClicked);
  connect(pause, &QPushButton::clicked, this, &PlayerPane::pauseClicked);
  connect(stop, &QPushButton::clicked, this, &PlayerPane::stopClicked);
  connect(download, &QPushButton::clicked, this, &PlayerPane::downloadClicked);

  connect(progress_slider_, &QSlider::sliderPressed, this,
          &PlayerPane::onSliderPressed);
  connect(progress_slider_, &QSlider::sliderReleased, this,
          &PlayerPane::onSliderReleased);
  connect(progress_slider_, &QSlider::sliderMoved, this,
          &PlayerPane::onSliderMoved);
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

void PlayerPane::setPlaybackProgress(double position_sec, double duration_sec) {
  duration_sec_ = duration_sec;
  time_current_->setText(FormatTime(position_sec));
  if (duration_sec > 0.0) {
    time_total_->setText(FormatTime(duration_sec));
    if (!scrubbing_) {
      const int slider_val =
          static_cast<int>((position_sec / duration_sec) * 1000.0 + 0.5);
      progress_slider_->setValue(qBound(0, slider_val, 1000));
    }
  } else {
    time_total_->setText(QString::fromUtf8("--:--"));
    if (!scrubbing_) {
      progress_slider_->setValue(0);
    }
  }
}

void PlayerPane::resetPlaybackProgress() {
  duration_sec_ = 0.0;
  scrubbing_ = false;
  progress_slider_->setValue(0);
  time_current_->setText(QString::fromUtf8("0:00"));
  time_total_->setText(QString::fromUtf8("0:00"));
}

void PlayerPane::onSliderPressed() {
  scrubbing_ = true;
  emit scrubbingChanged(true);
}

void PlayerPane::onSliderReleased() {
  scrubbing_ = false;
  emit scrubbingChanged(false);
  if (duration_sec_ > 0.0) {
    const double pos =
        (static_cast<double>(progress_slider_->value()) / 1000.0) * duration_sec_;
    time_current_->setText(FormatTime(pos));
    emit seekRequested(pos);
  }
}

void PlayerPane::onSliderMoved(int value) {
  if (!scrubbing_ || duration_sec_ <= 0.0) {
    return;
  }
  const double pos = (static_cast<double>(value) / 1000.0) * duration_sec_;
  time_current_->setText(FormatTime(pos));
}

}  // namespace client
}  // namespace square_dance
