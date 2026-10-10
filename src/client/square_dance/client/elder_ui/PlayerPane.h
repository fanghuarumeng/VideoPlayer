#pragma once

#include <QWidget>

class QLabel;
class QSlider;

namespace square_dance {
namespace client {

class VideoPlayerWidget;

class PlayerPane : public QWidget {
  Q_OBJECT

 public:
  explicit PlayerPane(QWidget* parent = 0);

  VideoPlayerWidget* playerWidget() const;
  void setPlaybackProgress(double position_sec, double duration_sec);
  void resetPlaybackProgress();

 signals:
  void playClicked();
  void pauseClicked();
  void stopClicked();
  void downloadClicked();
  void seekRequested(double position_sec);
  void scrubbingChanged(bool scrubbing);

 private slots:
  void onSliderPressed();
  void onSliderReleased();
  void onSliderMoved(int value);

 private:
  static QString FormatTime(double seconds);

  VideoPlayerWidget* player_;
  QSlider* progress_slider_;
  QLabel* time_current_;
  QLabel* time_total_;
  double duration_sec_;
  bool scrubbing_;
};

}  // namespace client
}  // namespace square_dance
