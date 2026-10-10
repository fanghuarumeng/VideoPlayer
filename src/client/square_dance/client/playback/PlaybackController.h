#pragma once

#include "square_dance/client/api/Video.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

class QAudioOutput;
class QIODevice;

namespace square_dance {
namespace client {

class FfmpegPlaybackEngine;
class VideoPlayerWidget;

class PlaybackController : public QObject {
  Q_OBJECT

 public:
  explicit PlaybackController(QObject* parent = 0);
  ~PlaybackController();

  void setPlayerWidget(VideoPlayerWidget* widget);
  void play(const Video& video);
  void pause();
  void stop();
  void seekTo(double position_sec);

 signals:
  void errorOccurred(const QString& message);
  void statusMessage(const QString& message);
  void playbackProgress(double position_sec, double duration_sec);

 public slots:
  void setScrubbing(bool scrubbing);

 private slots:
  void onVideoFrame(const QImage& frame, double pts_sec);
  void onAudioSamples(const QByteArray& pcm, double pts_sec);
  void onEngineFailed(const QString& message);
  void onEngineEnded();
  void onDurationReady(double duration_sec);
  void onPositionChanged(double position_sec);
  void onSeekCompleted(double position_sec);

 private:
  PlaybackController(const PlaybackController&);
  PlaybackController& operator=(const PlaybackController&);

  QUrl ResolvePlayUrl(const QString& url_text) const;
  void UpdateHint(const QString& text);
  void TearDownAudio();
  bool EnsureAudioOutput(QString* error);
  void EmitProgress();

  QPointer<VideoPlayerWidget> player_widget_;
  FfmpegPlaybackEngine* engine_;
  QAudioOutput* audio_out_;
  QIODevice* audio_device_;
  bool paused_;
  bool playing_;
  bool has_presented_frame_;
  bool scrubbing_;
  double duration_sec_;
  double position_sec_;
  QString pending_title_;
};

}  // namespace client
}  // namespace square_dance
