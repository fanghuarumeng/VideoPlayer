#include "video_player/client/elder_ui/BufferedSeekSlider.h"

#include "video_player/client/elder_ui/AppTheme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionSlider>

namespace video_player {
namespace client {

BufferedSeekSlider::BufferedSeekSlider(Qt::Orientation orientation,
                                       QWidget* parent)
    : QSlider(orientation, parent), buffer_ratio_(0.0), dragging_(false) {
  setRange(0, 1000);
  setMouseTracking(true);
  setCursor(Qt::PointingHandCursor);
  // Groove/handle drawn in paintEvent; keep stylesheet empty for reliable buffer.
  setStyleSheet(QString());
  setToolTip(QString::fromUtf8(
      "蓝色：已播放位置\n米色：已缓冲/已下载\n深灰：尚未加载"));
}

void BufferedSeekSlider::setBufferRatio(double ratio) {
  const double clamped = qBound(0.0, ratio, 1.0);
  if (qAbs(clamped - buffer_ratio_) < 0.001) {
    return;
  }
  buffer_ratio_ = clamped;
  update();
}

double BufferedSeekSlider::bufferRatio() const {
  return buffer_ratio_;
}

double BufferedSeekSlider::RatioFromPos(const QPoint& pos) const {
  QStyleOptionSlider opt;
  initStyleOption(&opt);
  const QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderGroove, this);
  const QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderHandle, this);
  if (orientation() == Qt::Horizontal) {
    const int span = qMax(1, groove.width() - handle.width());
    const int x = pos.x() - groove.x() - handle.width() / 2;
    return qBound(0.0, static_cast<double>(x) / static_cast<double>(span), 1.0);
  }
  const int span = qMax(1, groove.height() - handle.height());
  const int y = pos.y() - groove.y() - handle.height() / 2;
  return qBound(0.0, 1.0 - static_cast<double>(y) / static_cast<double>(span),
               1.0);
}

void BufferedSeekSlider::ApplyRatio(double ratio, bool commit) {
  const int val =
      minimum() +
      static_cast<int>(ratio * (maximum() - minimum()) + 0.5);
  setValue(qBound(minimum(), val, maximum()));
  if (commit) {
    emit seekCommitted(ratio);
  }
}

void BufferedSeekSlider::paintEvent(QPaintEvent* /*event*/) {
  QStyleOptionSlider opt;
  initStyleOption(&opt);
  QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt,
                                         QStyle::SC_SliderGroove, this);
  QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt,
                                         QStyle::SC_SliderHandle, this);

  // Thin mock-like groove.
  const int gh = 4;
  groove.setTop(groove.center().y() - gh / 2);
  groove.setHeight(gh);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  // Tokens from dark_player.qss: track 20% white, played #5086FF.
  const QColor track(255, 255, 255, 51);
  const QColor buffered(255, 255, 255, 90);
  const QColor played(0x50, 0x86, 0xff);
  const QColor knob_fill(0xff, 0xff, 0xff);

  p.setPen(Qt::NoPen);
  p.setBrush(track);
  p.drawRoundedRect(groove, 2, 2);

  if (buffer_ratio_ > 0.0) {
    QRect buf = groove;
    buf.setWidth(qMax(0, static_cast<int>(groove.width() * buffer_ratio_)));
    p.setBrush(buffered);
    p.drawRoundedRect(buf, 2, 2);
  }

  const double play_ratio =
      (maximum() > minimum())
          ? static_cast<double>(value() - minimum()) /
                static_cast<double>(maximum() - minimum())
          : 0.0;
  if (play_ratio > 0.0) {
    QRect play = groove;
    play.setWidth(qMax(0, static_cast<int>(groove.width() * play_ratio)));
    p.setBrush(played);
    p.drawRoundedRect(play, 2, 2);
  }

  const int hs = 12;
  QRect knob(0, 0, hs, hs);
  knob.moveCenter(QPoint(
      groove.x() + static_cast<int>(groove.width() * play_ratio),
      groove.center().y()));
  p.setBrush(knob_fill);
  p.setPen(Qt::NoPen);
  p.drawEllipse(knob);
  Q_UNUSED(handle);
}

void BufferedSeekSlider::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    dragging_ = true;
    setSliderDown(true);  // emits sliderPressed
    ApplyRatio(RatioFromPos(event->pos()), true);
    event->accept();
    return;
  }
  QSlider::mousePressEvent(event);
}

void BufferedSeekSlider::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_ && (event->buttons() & Qt::LeftButton)) {
    ApplyRatio(RatioFromPos(event->pos()), false);
    event->accept();
    return;
  }
  QSlider::mouseMoveEvent(event);
}

void BufferedSeekSlider::mouseReleaseEvent(QMouseEvent* event) {
  if (dragging_ && event->button() == Qt::LeftButton) {
    ApplyRatio(RatioFromPos(event->pos()), true);
    dragging_ = false;
    setSliderDown(false);  // emits sliderReleased
    event->accept();
    return;
  }
  QSlider::mouseReleaseEvent(event);
}

}  // namespace client
}  // namespace video_player
