#pragma once

#include "square_dance/client/qml/PlayerUiBridge.h"

#include <QImage>
#include <QQuickPaintedItem>

namespace square_dance {
namespace client {

class VideoFrameItem : public QQuickPaintedItem {
  Q_OBJECT
  Q_PROPERTY(PlayerUiBridge* bridge READ bridge WRITE setBridge NOTIFY
                 bridgeChanged)
  Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)

 public:
  explicit VideoFrameItem(QQuickItem* parent = 0);

  PlayerUiBridge* bridge() const;
  void setBridge(PlayerUiBridge* bridge);

  bool fill() const;
  void setFill(bool fill);

  void paint(QPainter* painter);

 signals:
  void bridgeChanged();
  void fillChanged();

 private slots:
  void onFrameChanged();

 private:
  PlayerUiBridge* bridge_;
  bool fill_;
};

}  // namespace client
}  // namespace square_dance
