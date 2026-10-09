#include "square_dance/client/playback/PlaybackController.h"

#include <QMediaPlayer>
#include <QUrl>
#include <QVideoWidget>

namespace square_dance {
namespace client {

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent), player_(new QMediaPlayer(this)) {
  connect(player_,
          static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(
              &QMediaPlayer::error),
          this, &PlaybackController::onMediaError);
  connect(player_, &QMediaPlayer::mediaStatusChanged, this,
          &PlaybackController::onMediaStatusChanged);
}

void PlaybackController::setVideoOutput(QVideoWidget* widget) {
  player_->setVideoOutput(widget);
}

void PlaybackController::play(const Video& video) {
  const QString url = video.play_url.trimmed();
  if (url.isEmpty()) {
    emit errorOccurred(QString::fromUtf8("播放地址为空"));
    return;
  }
  const QUrl media_url(url);
  if (!media_url.isValid() || media_url.scheme().isEmpty()) {
    emit errorOccurred(QString::fromUtf8("播放地址无效"));
    return;
  }
  player_->setMedia(media_url);
  player_->play();
}

void PlaybackController::pause() { player_->pause(); }

void PlaybackController::stop() { player_->stop(); }

void PlaybackController::onMediaStatusChanged() {
  if (player_->mediaStatus() == QMediaPlayer::InvalidMedia) {
    emit errorOccurred(QString::fromUtf8("无法播放该视频"));
  }
}

void PlaybackController::onMediaError() {
  const QString detail = player_->errorString();
  if (detail.isEmpty()) {
    emit errorOccurred(QString::fromUtf8("播放失败"));
  } else {
    emit errorOccurred(detail);
  }
}

}  // namespace client
}  // namespace square_dance
