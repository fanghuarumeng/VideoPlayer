#include "square_dance/client/elder_ui/BufferedSeekSlider.h"

#include "square_dance/client/elder_ui/AppTheme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionSlider>

namespace square_dance {
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
      "蓝色：已播放位置\n灰色：已缓冲/已下载\n浅灰：尚未加载"));
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

  // Center a compact groove.
  const int gh = 10;
  groove.setTop(groove.center().y() - gh / 2);
  groove.setHeight(gh);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  // YouTube-style mapping (high contrast, not two blues):
  //   light gray  = not fetched yet
  //   slate gray  = already buffered/downloaded
  //   accent blue = already played (under the knob)
  const QColor track(0xe2, 0xe8, 0xf0);     // unused remainder
  const QColor buffered(0x94, 0xa3, 0xb8);  // downloaded
  const QColor played(0x02, 0x84, 0xc7);    // playhead fill
  const QColor knob_fill(0xff, 0xff, 0xff);

  p.setPen(Qt::NoPen);
  p.setBrush(track);
  p.drawRoundedRect(groove, 5, 5);

  if (buffer_ratio_ > 0.0) {
    QRect buf = groove;
    buf.setWidth(qMax(0, static_cast<int>(groove.width() * buffer_ratio_)));
    p.setBrush(buffered);
    p.drawRoundedRect(buf, 5, 5);
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
    p.drawRoundedRect(play, 5, 5);
  }

  const int hs = 16;
  QRect knob(0, 0, hs, hs);
  knob.moveCenter(QPoint(
      groove.x() + static_cast<int>(groove.width() * play_ratio),
      groove.center().y()));
  p.setBrush(knob_fill);
  p.setPen(QPen(played, 2));
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
}  // namespace square_dance
