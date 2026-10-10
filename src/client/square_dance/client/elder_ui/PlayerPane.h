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
  void setTitle(const QString& title);

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
  void UpdateTimeLabel(double position_sec, double duration_sec);

  VideoPlayerWidget* player_;
  QLabel* title_label_;
  QLabel* time_label_;
  BufferedSeekSlider* progress_slider_;
  QPushButton* play_btn_;
  QPushButton* pause_btn_;
  QPushButton* stop_btn_;
  QPushButton* download_btn_;
  double duration_sec_;
  double position_sec_;
  bool scrubbing_;
};

}  // namespace client
}  // namespace square_dance
