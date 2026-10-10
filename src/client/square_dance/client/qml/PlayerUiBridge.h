#pragma once

#include "square_dance/client/ClientConfig.h"
#include "square_dance/client/api/Video.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QImage>
#include <QObject>
#include <QString>

namespace square_dance {
namespace client {

class CatalogClient;
class CatalogModel;
class DownloadService;
class HistoryModel;
class PlaybackController;
class PlayHistoryStore;
class PlaylistModel;
class PushClient;
class SecondaryPaneHost;
class UploadController;

class PlayerUiBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString title READ title NOTIFY titleChanged)
  Q_PROPERTY(QString hintText READ hintText NOTIFY hintTextChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(double positionSec READ positionSec NOTIFY progressChanged)
  Q_PROPERTY(double durationSec READ durationSec NOTIFY progressChanged)
  Q_PROPERTY(double bufferRatio READ bufferRatio NOTIFY bufferChanged)
  Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
  Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY frameChanged)
  Q_PROPERTY(QObject* playlistModel READ playlistModelConstant NOTIFY
                 playlistModelChanged)
  Q_PROPERTY(QObject* libraryModel READ libraryModelConstant NOTIFY
                 libraryModelChanged)
  Q_PROPERTY(QObject* searchModel READ searchModelConstant NOTIFY
                 searchModelChanged)
  Q_PROPERTY(QObject* historyModel READ historyModelConstant NOTIFY
                 historyModelChanged)
  Q_PROPERTY(bool liveConnected READ liveConnected NOTIFY liveChanged)
  Q_PROPERTY(QString overlay READ overlay NOTIFY overlayChanged)

 public:
  explicit PlayerUiBridge(const ClientConfig& config, QObject* parent = 0);
  ~PlayerUiBridge();

  QString title() const;
  QString hintText() const;
  QString statusText() const;
  double positionSec() const;
  double durationSec() const;
  double bufferRatio() const;
  bool paused() const;
  bool playing() const;
  bool hasFrame() const;
  bool liveConnected() const;
  QString overlay() const;
  QObject* playlistModelConstant() const;
  QObject* libraryModelConstant() const;
  QObject* searchModelConstant() const;
  QObject* historyModelConstant() const;
  QImage currentFrame() const;

  Q_INVOKABLE void playSelectedOrResume();
  Q_INVOKABLE void pauseToggle();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void seekRatio(double ratio);
  Q_INVOKABLE void setScrubbing(bool scrubbing);
  Q_INVOKABLE void playPlaylistIndex(int index);
  Q_INVOKABLE void clearPlaylist();
  Q_INVOKABLE void downloadCurrent();
  Q_INVOKABLE void openLibrary();
  Q_INVOKABLE void openHistory();
  Q_INVOKABLE void openSearch();
  Q_INVOKABLE void closeOverlay();
  Q_INVOKABLE void refreshLibrary();
  Q_INVOKABLE void uploadVideos();
  Q_INVOKABLE void playLibraryAt(int index);
  Q_INVOKABLE void addLibraryAt(int index);
  Q_INVOKABLE void searchVideos(const QString& query);
  Q_INVOKABLE void playSearchAt(int index);
  Q_INVOKABLE void addSearchAt(int index);
  Q_INVOKABLE void playHistoryAt(int index);
  Q_INVOKABLE void clearHistory();

 signals:
  void titleChanged();
  void hintTextChanged();
  void statusTextChanged();
  void progressChanged();
  void bufferChanged();
  void pausedChanged();
  void playingChanged();
  void frameChanged();
  void playlistModelChanged();
  void libraryModelChanged();
  void searchModelChanged();
  void historyModelChanged();
  void liveChanged();
  void overlayChanged();

 private slots:
  void onPlaybackProgress(double position_sec, double duration_sec);
  void onBufferProgress(double buffered_ratio);
  void onPausedChanged(bool paused);
  void onStatusMessage(const QString& message);
  void onPlaybackError(const QString& message);
  void onPlaybackFinished();
  void onVideoFrame(const QImage& frame);
  void onListFinished(const QVector<Video>& videos);
  void onVideoUpserted(const Video& video);
  void onHistoryPlay(const Video& video, qint64 position_ms);
  void onLibraryPlay(const Video& video);
  void onConnectionChanged(bool connected);
  void onSearchFinished(const QVector<Video>& videos);

 private:
  void PlayVideo(const Video& video, qint64 seek_ms = -1);
  void SetTitle(const QString& title);
  void SetHint(const QString& hint);
  void SetStatus(const QString& status);
  void SetPlaying(bool playing);
  void SetOverlay(const QString& overlay);
  QString ResolvePlaylistPath() const;
  QString ResolveHistoryPath() const;
  Video VideoFromPlaylistItem(const PlaylistItem& item) const;

  ClientConfig config_;
  PlaybackController* playback_;
  PlaylistStore* playlist_store_;
  PlaylistModel* playlist_model_;
  CatalogClient* catalog_;
  CatalogModel* catalog_model_;
  CatalogModel* library_model_;
  HistoryModel* history_model_;
  PushClient* push_;
  DownloadService* downloads_;
  UploadController* upload_;
  SecondaryPaneHost* panes_;
  PlayHistoryStore* history_store_;

  QString title_;
  QString hint_text_;
  QString status_text_;
  QString overlay_;
  double position_sec_;
  double duration_sec_;
  double buffer_ratio_;
  bool paused_;
  bool playing_;
  bool live_connected_;
  bool playing_from_playlist_;
  qint64 last_history_save_ms_;
  QImage current_frame_;
  Video current_video_;
};

}  // namespace client
}  // namespace square_dance
