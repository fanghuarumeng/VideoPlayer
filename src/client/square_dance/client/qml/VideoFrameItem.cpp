#include "square_dance/client/qml/VideoFrameItem.h"

#include "square_dance/client/qml/PlayerUiBridge.h"

#include <QPainter>

namespace square_dance {
namespace client {

VideoFrameItem::VideoFrameItem(QQuickItem* parent)
    : QQuickPaintedItem(parent), bridge_(NULL), fill_(true) {
  setRenderTarget(QQuickPaintedItem::FramebufferObject);
  setAntialiasing(true);
  setOpaquePainting(true);
}

PlayerUiBridge* VideoFrameItem::bridge() const { return bridge_; }

void VideoFrameItem::setBridge(PlayerUiBridge* bridge) {
  if (bridge_ == bridge) {
    return;
  }
  if (bridge_ != NULL) {
    disconnect(bridge_, 0, this, 0);
  }
  bridge_ = bridge;
  if (bridge_ != NULL) {
    connect(bridge_, &PlayerUiBridge::frameChanged, this,
            &VideoFrameItem::onFrameChanged);
  }
  emit bridgeChanged();
  update();
}

bool VideoFrameItem::fill() const { return fill_; }

void VideoFrameItem::setFill(bool fill) {
  if (fill_ == fill) {
    return;
  }
  fill_ = fill;
  emit fillChanged();
  update();
}

void VideoFrameItem::onFrameChanged() { update(); }

void VideoFrameItem::paint(QPainter* painter) {
  painter->fillRect(boundingRect(), QColor(0, 0, 0));
  if (bridge_ == NULL) {
    return;
  }
  const QImage frame = bridge_->currentFrame();
  if (frame.isNull()) {
    return;
  }
  const QRectF dest = boundingRect();
  if (fill_) {
    painter->drawImage(dest, frame);
    return;
  }
  const QSizeF fitted =
      QSizeF(frame.size()).scaled(dest.size(), Qt::KeepAspectRatio);
  QRectF target(0, 0, fitted.width(), fitted.height());
  target.moveCenter(dest.center());
  painter->drawImage(target, frame);
}

}  // namespace client
}  // namespace square_dance
