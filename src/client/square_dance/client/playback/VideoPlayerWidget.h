#pragma once

#include <QWidget>

class QVideoWidget;
class QVBoxLayout;

namespace square_dance {
namespace client {

class VideoPlayerWidget : public QWidget {
  Q_OBJECT

 public:
  explicit VideoPlayerWidget(QWidget* parent = 0);

  QVideoWidget* videoWidget() const;

 private:
  QVideoWidget* video_widget_;
};

}  // namespace client
}  // namespace square_dance
