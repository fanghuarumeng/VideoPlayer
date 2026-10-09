#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QVideoWidget>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QWidget(parent), video_widget_(new QVideoWidget(this)) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(video_widget_);
}

QVideoWidget* VideoPlayerWidget::videoWidget() const { return video_widget_; }

}  // namespace client
}  // namespace square_dance
