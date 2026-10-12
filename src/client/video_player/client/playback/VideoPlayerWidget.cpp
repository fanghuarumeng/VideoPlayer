#include "video_player/client/playback/VideoPlayerWidget.h"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSizePolicy>

namespace video_player {
namespace client {

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent),
      hint_(new QLabel(this)),
      loading_cover_(new QLabel(this)),
      show_hint_(true) {
  setMinimumSize(360, 240);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAutoFillBackground(false);

  hint_->setAlignment(Qt::AlignCenter);
  hint_->setWordWrap(true);
  hint_->setStyleSheet(
      QString::fromUtf8("QLabel { color: #94a3b8; background: transparent; "
                        "font-size: 11pt; padding: 24px; }"));
  setHintText(QString::fromUtf8("双击列表条目，或点击播放"));

  loading_cover_->setAlignment(Qt::AlignCenter);
  loading_cover_->setWordWrap(true);
  loading_cover_->setStyleSheet(
      QString::fromUtf8("QLabel { color: #94a3b8; background: #0b0f14; "
                        "font-size: 11pt; }"));
  loading_cover_->setText(QString::fromUtf8("加载中…"));
  loading_cover_->hide();
}

void VideoPlayerWidget::setHintText(const QString& text) { hint_->setText(text); }

void VideoPlayerWidget::showHintPage() {
  setLoadingCoverVisible(false);
  show_hint_ = true;
  hint_->show();
  hint_->raise();
  clearFrame();
}

void VideoPlayerWidget::clearFrame() {
  frame_ = QImage();
  update();
}

void VideoPlayerWidget::setFrame(const QImage& frame) {
  if (frame.isNull()) {
    return;
  }
  show_hint_ = false;
  hint_->hide();
  frame_ = frame;
  update();
}

void VideoPlayerWidget::setLoadingCoverVisible(bool visible) {
  if (!visible) {
    loading_cover_->hide();
    return;
  }
  loading_cover_->setGeometry(rect());
  loading_cover_->raise();
  loading_cover_->show();
}

void VideoPlayerWidget::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter(this);
  painter.fillRect(rect(), QColor(0, 0, 0));
  if (!frame_.isNull()) {
    // Stretch to fill the player pane (满窗口).
    painter.drawImage(rect(), frame_);
  }
}

void VideoPlayerWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  hint_->setGeometry(rect());
  if (loading_cover_->isVisible()) {
    loading_cover_->setGeometry(rect());
    loading_cover_->raise();
  }
  if (show_hint_) {
    hint_->raise();
  }
}

}  // namespace client
}  // namespace video_player
