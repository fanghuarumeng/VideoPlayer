#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace square_dance {
namespace client {

class BufferedSeekSlider;
class VideoPlayerWidget;

class PlayerPane : public QWidget {
  Q_OBJECT

 public:
  explicit PlayerPane(QWidget* parent = 0);

  VideoPlayerWidget* playerWidget() const;
  void setPlaybackProgress(double position_sec, double duration_sec);
  void setBufferProgress(double buffered_ratio);
  void resetPlaybackProgress();
  void setPausedState(bool paused);

 signals:
  void playClicked();
  void pauseClicked();
  void stopClicked();
  void downloadClicked();
  void seekRequested(double position_sec);
  void scrubbingChanged(bool scrubbing);

 private slots:
  void onSeekCommitted(double ratio);

 private:
  static QString FormatTime(double seconds);

  VideoPlayerWidget* player_;
  BufferedSeekSlider* progress_slider_;
  QLabel* time_current_;
  QLabel* time_total_;
  QPushButton* pause_btn_;
  double duration_sec_;
  bool scrubbing_;
};

}  // namespace client
}  // namespace square_dance
