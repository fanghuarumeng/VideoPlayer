#pragma once

#include "square_dance/client/ClientConfig.h"
#include "square_dance/client/api/Video.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QMainWindow>
#include <QString>
#include <QVector>

class QLabel;
class QTabWidget;
class QWidget;

namespace square_dance {
namespace client {

class CatalogClient;
class CatalogModel;
class ConnectionStatusWidget;
class DownloadService;
class LibraryPane;
class PlaybackController;
class PlayerPane;
class PlaylistPane;
class PlaylistStore;
class PushClient;
class SearchBar;
class VideoResultList;

class ElderMainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit ElderMainWindow(const ClientConfig& config, QWidget* parent = 0);

 private slots:
  void onSearchRequested(const QString& query);
  void onSearchFinished(const QVector<square_dance::client::Video>& videos);
  void onSearchFailed(const QString& error);
  void onListFinished(const QVector<square_dance::client::Video>& videos);
  void onListFailed(const QString& error);
  void onPlayClicked();
  void onPauseClicked();
  void onStopClicked();
  void onDownloadClicked();
  void onPlaybackError(const QString& message);
  void onDownloadProgress(qint64 video_id, qint64 received, qint64 total);
  void onDownloadFinished(qint64 video_id, const QString& local_path);
  void onDownloadError(qint64 video_id, const QString& message);
  void onConnectionChanged(bool connected);
  void onPlaylistPlayRequested(const PlaylistItem& item);
  void onPlaybackFinished();
  void onAddSearchToPlaylist();
  void onLibraryRefresh();
  void onLibraryAddToPlaylist(const QVector<Video>& videos);
  void onLibraryPlayRequested(const Video& video);
  void onVideoUpserted(const Video& video);
  void onVideoRemoved(qint64 id);
  void SetStatusMessage(const QString& message);

 private:
  bool SelectedSearchVideo(Video* out) const;
  Video VideoFromPlaylistItem(const PlaylistItem& item) const;
  QString ResolvePlaylistPath() const;
  void PlayVideo(const Video& video);

  ClientConfig config_;
  SearchBar* search_bar_;
  VideoResultList* result_list_;
  PlayerPane* player_pane_;
  ConnectionStatusWidget* connection_status_;
  QLabel* status_label_;
  QTabWidget* side_tabs_;
  PlaylistStore* playlist_store_;
  PlaylistPane* playlist_pane_;
  LibraryPane* library_pane_;
  QWidget* history_placeholder_;

  CatalogModel* model_;
  CatalogClient* catalog_;
  PushClient* push_;
  PlaybackController* playback_;
  DownloadService* downloads_;
  QString current_query_;
  bool playing_from_playlist_;
};

}  // namespace client
}  // namespace square_dance
