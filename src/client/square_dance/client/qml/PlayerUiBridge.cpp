#include "square_dance/client/qml/PlayerUiBridge.h"

#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/api/PushClient.h"
#include "square_dance/client/download/DownloadService.h"
#include "square_dance/client/history/PlayHistoryStore.h"
#include "square_dance/client/library/UploadController.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playlist/PlaylistModel.h"
#include "square_dance/client/qml/SecondaryPaneHost.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QWidget>

namespace square_dance {
namespace client {

PlayerUiBridge::PlayerUiBridge(const ClientConfig& config, QObject* parent)
    : QObject(parent),
      config_(config),
      playback_(new PlaybackController(this)),
      playlist_store_(NULL),
      playlist_model_(NULL),
      catalog_(new CatalogClient(config.http_base_url, this)),
      catalog_model_(new CatalogModel(this)),
      push_(new PushClient(this)),
      downloads_(new DownloadService(config.download_dir, this)),
      upload_(new UploadController(config.http_base_url, 2, this)),
      panes_(NULL),
      history_store_(NULL),
      hint_text_(QString::fromUtf8("双击列表条目，或点击播放")),
      status_text_(QString::fromUtf8("正在加载视频库…")),
      position_sec_(0.0),
      duration_sec_(0.0),
      buffer_ratio_(0.0),
      paused_(false),
      playing_(false),
      live_connected_(false),
      playing_from_playlist_(false),
      last_history_save_ms_(0) {
  playlist_store_ = new PlaylistStore(ResolvePlaylistPath());
  QString pl_err;
  playlist_store_->load(&pl_err);
  playlist_model_ = new PlaylistModel(playlist_store_, this);
  playlist_model_->reload();

  history_store_ = new PlayHistoryStore(ResolveHistoryPath(), 100);
  QString hist_err;
  history_store_->load(&hist_err);

  panes_ = new SecondaryPaneHost(config_, catalog_, catalog_model_,
                                 playlist_store_, history_store_, upload_, this);

  connect(playback_, &PlaybackController::playbackProgress, this,
          &PlayerUiBridge::onPlaybackProgress);
  connect(playback_, &PlaybackController::bufferProgress, this,
          &PlayerUiBridge::onBufferProgress);
  connect(playback_, &PlaybackController::pausedChanged, this,
          &PlayerUiBridge::onPausedChanged);
  connect(playback_, &PlaybackController::statusMessage, this,
          &PlayerUiBridge::onStatusMessage);
  connect(playback_, &PlaybackController::errorOccurred, this,
          &PlayerUiBridge::onPlaybackError);
  connect(playback_, &PlaybackController::playbackFinished, this,
          &PlayerUiBridge::onPlaybackFinished);
  connect(playback_, &PlaybackController::videoFrameReady, this,
          &PlayerUiBridge::onVideoFrame);

  connect(catalog_, &CatalogClient::listFinished, this,
          &PlayerUiBridge::onListFinished);
  connect(catalog_, &CatalogClient::listFailed, this,
          [this](const QString& err) {
            SetStatus(QString::fromUtf8("加载视频库失败：") + err);
          });
  connect(push_, &PushClient::videoUpserted, this,
          &PlayerUiBridge::onVideoUpserted);
  connect(push_, &PushClient::connectionChanged, this,
          &PlayerUiBridge::onConnectionChanged);
  connect(panes_, &SecondaryPaneHost::libraryPlayRequested, this,
          &PlayerUiBridge::onLibraryPlay);
  connect(panes_, &SecondaryPaneHost::historyPlayRequested, this,
          &PlayerUiBridge::onHistoryPlay);
  connect(panes_, &SecondaryPaneHost::statusMessage, this,
          &PlayerUiBridge::onStatusMessage);
  connect(panes_, &SecondaryPaneHost::playlistChanged, this, [this]() {
    playlist_model_->reload();
    emit playlistModelChanged();
  });
  connect(upload_, &UploadController::statusMessage, this,
          &PlayerUiBridge::onStatusMessage);
  connect(upload_, &UploadController::uploadFinished, this,
          &PlayerUiBridge::onVideoUpserted);
  connect(downloads_, &DownloadService::progress, this,
          [this](qint64, qint64 received, qint64 total) {
            if (total > 0) {
              SetStatus(QString::fromUtf8("下载进度：%1%").arg(
                  static_cast<int>((received * 100) / total)));
            } else {
              SetStatus(QString::fromUtf8("已下载 %1 字节").arg(received));
            }
          });
  connect(downloads_, &DownloadService::finished, this,
          [this](qint64, const QString& local_path) {
            SetStatus(QString::fromUtf8("下载完成：") + local_path);
          });
  connect(downloads_, &DownloadService::errorOccurred, this,
          [this](qint64, const QString& message) {
            SetStatus(QString::fromUtf8("下载失败：") + message);
          });

  push_->connectToServer(QUrl(config_.ws_url));
  catalog_->listVideos(200, 0);
}

PlayerUiBridge::~PlayerUiBridge() {}

QString PlayerUiBridge::title() const { return title_; }
QString PlayerUiBridge::hintText() const { return hint_text_; }
QString PlayerUiBridge::statusText() const { return status_text_; }
double PlayerUiBridge::positionSec() const { return position_sec_; }
double PlayerUiBridge::durationSec() const { return duration_sec_; }
double PlayerUiBridge::bufferRatio() const { return buffer_ratio_; }
bool PlayerUiBridge::paused() const { return paused_; }
bool PlayerUiBridge::playing() const { return playing_; }
bool PlayerUiBridge::hasFrame() const { return !current_frame_.isNull(); }
bool PlayerUiBridge::liveConnected() const { return live_connected_; }
QObject* PlayerUiBridge::playlistModelConstant() const {
  return playlist_model_;
}
QImage PlayerUiBridge::currentFrame() const { return current_frame_; }

QString PlayerUiBridge::ResolvePlaylistPath() const {
  if (!config_.playlist_path.isEmpty()) {
    return config_.playlist_path;
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/playlist.json");
}

QString PlayerUiBridge::ResolveHistoryPath() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/play_history.json");
}

Video PlayerUiBridge::VideoFromPlaylistItem(const PlaylistItem& item) const {
  Video video;
  video.id = item.id;
  video.title = item.title;
  video.play_url = item.play_url;
  video.download_url = item.download_url.isEmpty() ? item.play_url
                                                   : item.download_url;
  video.thumbnail_url = item.thumbnail_url;
  video.source_type = QString::fromUtf8("playlist");
  return video;
}

void PlayerUiBridge::SetTitle(const QString& title) {
  if (title_ == title) {
    return;
  }
  title_ = title;
  emit titleChanged();
}

void PlayerUiBridge::SetHint(const QString& hint) {
  if (hint_text_ == hint) {
    return;
  }
  hint_text_ = hint;
  emit hintTextChanged();
}

void PlayerUiBridge::SetStatus(const QString& status) {
  status_text_ = status;
  emit statusTextChanged();
}

void PlayerUiBridge::SetPlaying(bool playing) {
  if (playing_ == playing) {
    return;
  }
  playing_ = playing;
  emit playingChanged();
}

void PlayerUiBridge::PlayVideo(const Video& video, qint64 seek_ms) {
  current_video_ = video;
  SetTitle(video.title.isEmpty() ? QString::fromUtf8("正在播放") : video.title);
  SetHint(QString());
  SetStatus(QString::fromUtf8("正在播放：") + video.title);
  SetPlaying(true);
  current_frame_ = QImage();
  emit frameChanged();
  playback_->play(video);
  if (seek_ms > 0) {
    const double seek_sec = static_cast<double>(seek_ms) / 1000.0;
    QTimer::singleShot(400, playback_, [this, seek_sec]() {
      playback_->seekTo(seek_sec);
    });
  }
  last_history_save_ms_ = 0;
  if (history_store_ != NULL) {
    history_store_->recordProgress(video, seek_ms > 0 ? seek_ms : 0, 0);
  }
}

void PlayerUiBridge::playSelectedOrResume() {
  if (playing_ && paused_) {
    playback_->pause();
    return;
  }
  if (playlist_store_ != NULL && !playlist_store_->items().isEmpty()) {
    int idx = playlist_store_->currentIndex();
    if (idx < 0) {
      idx = 0;
    }
    playPlaylistIndex(idx);
    return;
  }
  if (current_video_.id != 0 || !current_video_.play_url.isEmpty()) {
    playing_from_playlist_ = false;
    PlayVideo(current_video_);
    return;
  }
  SetStatus(QString::fromUtf8("请先从视频库或播放列表选择视频"));
}

void PlayerUiBridge::pauseToggle() { playback_->pause(); }

void PlayerUiBridge::stop() {
  playing_from_playlist_ = false;
  playback_->stop();
  SetPlaying(false);
  position_sec_ = 0.0;
  duration_sec_ = 0.0;
  buffer_ratio_ = 0.0;
  current_frame_ = QImage();
  emit progressChanged();
  emit bufferChanged();
  emit frameChanged();
  SetHint(QString::fromUtf8("双击列表条目，或点击播放"));
}

void PlayerUiBridge::seekRatio(double ratio) {
  if (duration_sec_ <= 0.0) {
    return;
  }
  playback_->seekTo(qBound(0.0, ratio, 1.0) * duration_sec_);
}

void PlayerUiBridge::setScrubbing(bool scrubbing) {
  playback_->setScrubbing(scrubbing);
}

void PlayerUiBridge::playPlaylistIndex(int index) {
  if (playlist_store_ == NULL) {
    return;
  }
  const QVector<PlaylistItem> items = playlist_store_->items();
  if (index < 0 || index >= items.size()) {
    return;
  }
  playing_from_playlist_ = true;
  playlist_store_->setCurrentIndex(index);
  PlayVideo(VideoFromPlaylistItem(items[index]));
}

void PlayerUiBridge::clearPlaylist() {
  if (playlist_store_ == NULL) {
    return;
  }
  const int ret = QMessageBox::question(
      NULL, QString::fromUtf8("清空播放列表"),
      QString::fromUtf8("确定清空播放列表吗？"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (ret != QMessageBox::Yes) {
    return;
  }
  playlist_store_->clear();
  playlist_model_->reload();
  emit playlistModelChanged();
  SetStatus(QString::fromUtf8("已清空播放列表"));
}

void PlayerUiBridge::downloadCurrent() {
  Video video = current_video_;
  if (video.id == 0 && video.play_url.isEmpty() && playlist_store_ != NULL) {
    const QVector<PlaylistItem> items = playlist_store_->items();
    if (!items.isEmpty()) {
      int idx = playlist_store_->currentIndex();
      if (idx < 0 || idx >= items.size()) {
        idx = 0;
      }
      video = VideoFromPlaylistItem(items[idx]);
    }
  }
  if (video.id == 0 && video.play_url.isEmpty()) {
    SetStatus(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  if (video.download_url.trimmed().isEmpty()) {
    video.download_url = video.play_url;
  }
  if (video.download_url.trimmed().isEmpty()) {
    SetStatus(QString::fromUtf8("下载地址为空"));
    return;
  }

  const QString suggested = QDir(downloads_->download_dir())
                                .filePath(downloads_->defaultFileName(video));
  // Native dialog needs a QWidget parent; QML window is not one — raise a
  // transient host so the save dialog appears above the frameless shell.
  QWidget host;
  host.setWindowFlags(Qt::Tool);
  host.setAttribute(Qt::WA_DontShowOnScreen, true);
  host.resize(1, 1);
  host.show();
  const QString save_path = QFileDialog::getSaveFileName(
      &host, QString::fromUtf8("保存视频"), suggested,
      QString::fromUtf8("视频文件 (*.mp4);;所有文件 (*.*)"));
  host.hide();
  if (save_path.isEmpty()) {
    SetStatus(QString::fromUtf8("已取消下载"));
    return;
  }
  SetStatus(QString::fromUtf8("开始下载：") + video.title);
  downloads_->startDownload(video, save_path);
}

void PlayerUiBridge::openLibrary() { panes_->openLibrary(); }
void PlayerUiBridge::openHistory() { panes_->openHistory(); }
void PlayerUiBridge::openSearch() { panes_->openSearch(); }

void PlayerUiBridge::onPlaybackProgress(double position_sec,
                                        double duration_sec) {
  position_sec_ = position_sec;
  duration_sec_ = duration_sec;
  emit progressChanged();
  if (history_store_ != NULL && current_video_.id != 0) {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (last_history_save_ms_ == 0 || now - last_history_save_ms_ >= 5000) {
      history_store_->recordProgress(
          current_video_, static_cast<qint64>(position_sec * 1000.0),
          static_cast<qint64>(duration_sec * 1000.0));
      last_history_save_ms_ = now;
    }
  }
}

void PlayerUiBridge::onBufferProgress(double buffered_ratio) {
  buffer_ratio_ = buffered_ratio;
  emit bufferChanged();
}

void PlayerUiBridge::onPausedChanged(bool paused) {
  paused_ = paused;
  emit pausedChanged();
}

void PlayerUiBridge::onStatusMessage(const QString& message) {
  SetStatus(message);
}

void PlayerUiBridge::onPlaybackError(const QString& message) {
  SetPlaying(false);
  SetHint(message);
  SetStatus(QString::fromUtf8("播放错误：") + message);
}

void PlayerUiBridge::onPlaybackFinished() {
  SetPlaying(false);
  if (!playing_from_playlist_ || playlist_store_ == NULL) {
    return;
  }
  const int next = playlist_store_->currentIndex() + 1;
  if (next < 0 || next >= playlist_store_->items().size()) {
    SetStatus(QString::fromUtf8("播放列表已播完"));
    return;
  }
  playPlaylistIndex(next);
}

void PlayerUiBridge::onVideoFrame(const QImage& frame) {
  current_frame_ = frame;
  emit frameChanged();
}

void PlayerUiBridge::onListFinished(const QVector<Video>& videos) {
  panes_->setLibraryVideos(videos);
  bool playlist_changed = false;
  for (int i = 0; i < videos.size(); ++i) {
    if (playlist_store_->mergeFromVideo(videos[i])) {
      playlist_changed = true;
    }
  }
  if (playlist_changed) {
    playlist_model_->reload();
    emit playlistModelChanged();
  }
  SetStatus(QString::fromUtf8("视频库共 %1 个").arg(videos.size()));
}

void PlayerUiBridge::onVideoUpserted(const Video& video) {
  panes_->upsertLibraryVideo(video);
  if (playlist_store_->mergeFromVideo(video)) {
    playlist_model_->reload();
    emit playlistModelChanged();
  }
}

void PlayerUiBridge::onHistoryPlay(const Video& video, qint64 position_ms) {
  playing_from_playlist_ = false;
  PlayVideo(video, position_ms);
}

void PlayerUiBridge::onLibraryPlay(const Video& video) {
  playing_from_playlist_ = false;
  PlayVideo(video);
}

void PlayerUiBridge::onConnectionChanged(bool connected) {
  live_connected_ = connected;
  emit liveChanged();
  if (connected) {
    SetStatus(QString::fromUtf8("已连接实时推送"));
  } else {
    SetStatus(QString::fromUtf8("推送未连接，正在重试…"));
  }
}

}  // namespace client
}  // namespace square_dance
