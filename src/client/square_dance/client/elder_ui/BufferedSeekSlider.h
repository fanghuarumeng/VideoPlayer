#pragma once

#include <QSlider>

namespace square_dance {
namespace client {

// Seek slider with a secondary "buffered/downloaded" fill under the playhead.
// Clicking the groove jumps immediately (not only after a drag release).
class BufferedSeekSlider : public QSlider {
  Q_OBJECT

 public:
  explicit BufferedSeekSlider(Qt::Orientation orientation,
                              QWidget* parent = 0);

  // 0.0 .. 1.0 — how much of the media has been fetched/read.
  void setBufferRatio(double ratio);
  double bufferRatio() const;

 signals:
  void seekCommitted(double ratio);  // 0.0 .. 1.0 of media duration

 protected:
  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);

 private:
  double RatioFromPos(const QPoint& pos) const;
  void ApplyRatio(double ratio, bool commit);

  double buffer_ratio_;
  bool dragging_;
};

}  // namespace client
}  // namespace square_dance
