#pragma once

#include <QImage>
#include <QWidget>

class QLabel;

namespace square_dance {
namespace client {

// Single black surface + overlays. No page switching (avoids start-up flash).
class VideoPlayerWidget : public QWidget {
  Q_OBJECT

 public:
  explicit VideoPlayerWidget(QWidget* parent = 0);

 public slots:
  void setHintText(const QString& text);
  void showHintPage();
  void setFrame(const QImage& frame);
  void clearFrame();
  void setLoadingCoverVisible(bool visible);

 protected:
  void paintEvent(QPaintEvent* event);
  void resizeEvent(QResizeEvent* event);

 private:
  QImage frame_;
  QLabel* hint_;
  QLabel* loading_cover_;
  bool show_hint_;
};

}  // namespace client
}  // namespace square_dance
