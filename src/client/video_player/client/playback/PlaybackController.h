#pragma once

#include "video_player/client/api/Video.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtGlobal>

class QAudioOutput;
class QIODevice;

namespace video_player {
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
  void setPlaybackRate(double rate);
  double playbackRate() const;
  bool isPaused() const;

 signals:
  void errorOccurred(const QString& message);
  void statusMessage(const QString& message);
  void playbackProgress(double position_sec, double duration_sec);
  void bufferProgress(double buffered_ratio);
  void playbackFinished();
  void pausedChanged(bool paused);
  void playbackRateChanged(double rate);
  // Emitted for each presented video frame (QML shell / alternate surfaces).
  void videoFrameReady(const QImage& frame);

 public slots:
  void setScrubbing(bool scrubbing);

 private slots:
  void onVideoFrame(const QImage& frame, double pts_sec, quint64 job_id);
  void onAudioSamples(const QByteArray& pcm, double pts_sec, quint64 job_id);
  void onEngineFailed(const QString& message, quint64 job_id);
  void onEngineEnded(quint64 job_id);
  void onDurationReady(double duration_sec, quint64 job_id);
  void onPositionChanged(double position_sec, quint64 job_id);
  void onSeekCompleted(double position_sec, quint64 job_id);
  void onBufferProgress(double buffered_ratio, quint64 job_id);

 private:
  PlaybackController(const PlaybackController&);
  PlaybackController& operator=(const PlaybackController&);

  QUrl ResolvePlayUrl(const QString& url_text) const;
  void UpdateHint(const QString& text);
  void TearDownAudio();
  bool EnsureAudioOutput(QString* error);
  void EmitProgress();
  bool IsActiveJob(quint64 job_id) const;
  void SetPaused(bool paused);

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
  double playback_rate_;
  QString pending_title_;
  quint64 active_job_id_;
};

}  // namespace client
}  // namespace video_player
