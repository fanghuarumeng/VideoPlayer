#pragma once

#include <QWidget>

namespace square_dance {
namespace client {

class VideoPlayerWidget;

class PlayerPane : public QWidget {
  Q_OBJECT

 public:
  explicit PlayerPane(QWidget* parent = 0);

  VideoPlayerWidget* playerWidget() const;

 signals:
  void playClicked();
  void pauseClicked();
  void stopClicked();
  void downloadClicked();

 private:
  VideoPlayerWidget* player_;
};

}  // namespace client
}  // namespace square_dance
