#pragma once

#include "square_dance/client/api/Video.h"

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QUrl>

class QAudioOutput;
class QIODevice;
class QTimer;

namespace square_dance {
namespace client {

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

 signals:
  void errorOccurred(const QString& message);
  void statusMessage(const QString& message);

 private slots:
  void onVideoReadyRead();
  void onAudioReadyRead();
  void onFrameTick();
  void onVideoFinished(int exit_code, QProcess::ExitStatus status);
  void onAudioFinished(int exit_code, QProcess::ExitStatus status);

 private:
  PlaybackController(const PlaybackController&);
  PlaybackController& operator=(const PlaybackController&);

  QUrl ResolvePlayUrl(const QString& url_text) const;
  void UpdateHint(const QString& text);
  void StopPipelines();
  bool StartPipelines(const QUrl& media_url, const QString& title, QString* error);
  QSize ChooseFrameSize() const;
  bool PlayWithSystemPlayer(const QUrl& media_url,
                            const QString& title,
                            QString* error);
  bool SetProcessSuspended(QProcess* process, bool suspend);

  VideoPlayerWidget* player_widget_;
  QProcess* video_proc_;
  QProcess* audio_proc_;
  QAudioOutput* audio_out_;
  QIODevice* audio_device_;
  QTimer* frame_timer_;
  QByteArray video_buffer_;
  QList<QImage> frame_queue_;
  QString pending_title_;
  int frame_w_;
  int frame_h_;
  int frame_bytes_;
  bool paused_;
  bool playing_;
  bool has_presented_frame_;
};

}  // namespace client
}  // namespace square_dance
