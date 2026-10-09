#pragma once

#include "square_dance/client/api/Video.h"

#include <QObject>
#include <QString>

class QMediaPlayer;
class QVideoWidget;

namespace square_dance {
namespace client {

class PlaybackController : public QObject {
  Q_OBJECT

 public:
  explicit PlaybackController(QObject* parent = 0);

  void setVideoOutput(QVideoWidget* widget);
  void play(const Video& video);
  void pause();
  void stop();

 signals:
  void errorOccurred(const QString& message);

 private slots:
  void onMediaStatusChanged();
  void onMediaError();

 private:
  PlaybackController(const PlaybackController&);
  PlaybackController& operator=(const PlaybackController&);

  QMediaPlayer* player_;
};

}  // namespace client
}  // namespace square_dance
