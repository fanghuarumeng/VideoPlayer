#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include <QtGlobal>

namespace square_dance {
namespace client {

// Demux + decode on a worker thread (single AVFormatContext).
// Audio is converted to interleaved s16le stereo 44100 for QAudioOutput.
// Video frames are RGB888 QImages with PTS seconds.
// Every signal carries job_id so the controller can ignore stale QueuedConnection
// deliveries after stop/restart (rapid double-click play).
class FfmpegPlaybackEngine : public QThread {
  Q_OBJECT

 public:
  explicit FfmpegPlaybackEngine(QObject* parent = 0);
  ~FfmpegPlaybackEngine();

  // Returns the job id for this start, or 0 if the previous worker could not be
  // stopped in time.
  quint64 startPlayback(const QString& url);
  void requestStop();
  void setPaused(bool paused);
  void seekTo(double position_sec);
  bool ShouldAbort() const;
  quint64 currentJobId() const;

  static const int kAudioSampleRate = 44100;
  static const int kAudioChannels = 2;
  static const int kAudioBytesPerSample = 2;  // s16

 signals:
  void videoFrameReady(const QImage& frame, double pts_sec, quint64 job_id);
  void audioSamplesReady(const QByteArray& pcm, double pts_sec, quint64 job_id);
  void playbackFailed(const QString& message, quint64 job_id);
  void playbackEnded(quint64 job_id);
  void durationReady(double duration_sec, quint64 job_id);
  void positionChanged(double position_sec, quint64 job_id);
  void seekCompleted(double position_sec, quint64 job_id);
  // 0.0 .. 1.0 approximate fraction of the media body already fetched/read.
  void bufferProgress(double buffered_ratio, quint64 job_id);

 protected:
  void run();

 private:
  FfmpegPlaybackEngine(const FfmpegPlaybackEngine&);
  FfmpegPlaybackEngine& operator=(const FfmpegPlaybackEngine&);

  bool OpenInput(const QString& url, QString* error);
  void CloseInput();
  bool OpenCodecs(QString* error);
  void DecodeLoop(quint64 job_id);
  // Sleep so media PTS does not run ahead of wall clock (keeps A/V realtime).
  void PaceToPts(double pts_sec);
  double ComputeDurationSec() const;
  bool PerformSeek(double target_sec);
  void MaybeEmitPosition(double pts_sec, quint64 job_id);
  void MaybeEmitBuffer(quint64 job_id);

  mutable QMutex mutex_;
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
  double duration_sec_;
  double last_reported_position_sec_;
  double last_reported_buffer_ratio_;
  qint64 media_byte_size_;
  qint64 media_bytes_high_water_;
  bool pending_seek_;
  double pending_seek_sec_;
  quint64 job_id_;
  quint64 next_job_id_;

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
