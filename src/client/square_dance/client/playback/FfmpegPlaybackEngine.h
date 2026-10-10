#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>

namespace square_dance {
namespace client {

// Demux + decode on a worker thread (single AVFormatContext).
// Audio is converted to interleaved s16le stereo 44100 for QAudioOutput.
// Video frames are RGB888 QImages with PTS seconds.
class FfmpegPlaybackEngine : public QThread {
  Q_OBJECT

 public:
  explicit FfmpegPlaybackEngine(QObject* parent = 0);
  ~FfmpegPlaybackEngine();

  void startPlayback(const QString& url);
  void requestStop();
  void setPaused(bool paused);
  bool ShouldAbort() const;

  static const int kAudioSampleRate = 44100;
  static const int kAudioChannels = 2;
  static const int kAudioBytesPerSample = 2;  // s16

 signals:
  void videoFrameReady(const QImage& frame, double pts_sec);
  void audioSamplesReady(const QByteArray& pcm, double pts_sec);
  void playbackFailed(const QString& message);
  void playbackEnded();

 protected:
  void run();

 private:
  FfmpegPlaybackEngine(const FfmpegPlaybackEngine&);
  FfmpegPlaybackEngine& operator=(const FfmpegPlaybackEngine&);

  bool OpenInput(const QString& url, QString* error);
  void CloseInput();
  bool OpenCodecs(QString* error);
  void DecodeLoop();
  // Sleep so media PTS does not run ahead of wall clock (keeps A/V realtime).
  void PaceToPts(double pts_sec);

  QMutex mutex_;
  QWaitCondition pause_wait_;
  QString url_;
  bool stop_requested_;
  bool paused_;
  bool has_job_;
  bool clock_started_;
  double pts_origin_sec_;
  qint64 pause_accumulated_ms_;
  bool pause_wall_mark_valid_;
  qint64 pause_wall_mark_ms_;
  QElapsedTimer wall_timer_;

  // Opaque FFmpeg pointers kept as void* in header to avoid pulling C API into MOC.
  void* format_ctx_;
  void* video_codec_ctx_;
  void* audio_codec_ctx_;
  void* sws_ctx_;
  void* swr_ctx_;
  int video_stream_;
  int audio_stream_;
  int frame_w_;
  int frame_h_;
};

}  // namespace client
}  // namespace square_dance
