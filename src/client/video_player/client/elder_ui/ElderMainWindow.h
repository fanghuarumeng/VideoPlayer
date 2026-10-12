#pragma once

#include "video_player/client/ClientConfig.h"
#include "video_player/client/api/Video.h"
#include "video_player/client/playlist/PlaylistStore.h"

#include <QMainWindow>
#include <QString>
#include <QVector>

class QLabel;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace video_player {
namespace client {

class CatalogClient;
class CatalogModel;
class ConnectionStatusWidget;
class DownloadService;
class LibraryPane;
class UploadController;
class PlayHistoryStore;
class HistoryPane;
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
  void onSearchFinished(const QVector<video_player::client::Video>& videos);
  void onSearchFailed(const QString& error);
  void onListFinished(const QVector<video_player::client::Video>& videos);
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
  void onLibraryUpload();
  void onLibraryAddToPlaylist(const QVector<Video>& videos);
  void onLibraryPlayRequested(const Video& video);
  void onUploadFinished(const Video& video);
  void onVideoUpserted(const Video& video);
  void onVideoRemoved(qint64 id);
  void onPlaybackProgressForHistory(double position_sec, double duration_sec);
  void onHistoryPlayRequested(const Video& video, qint64 position_ms);
  void SetStatusMessage(const QString& message);
  void showPlayerPage();
  void showLibraryPage();
  void showHistoryPage();
  void showSearchPage();

 private:
  bool SelectedSearchVideo(Video* out) const;
  Video VideoFromPlaylistItem(const PlaylistItem& item) const;
  QString ResolvePlaylistPath() const;
  QString ResolveHistoryPath() const;
  void PlayVideo(const Video& video, qint64 seek_ms = -1);
  void RecordHistory(double position_sec, double duration_sec);
  QWidget* WrapSecondaryPage(QWidget* body, const QString& title);
  void SetNavChecked(QPushButton* active);

  ClientConfig config_;
  SearchBar* search_bar_;
  VideoResultList* result_list_;
  PlayerPane* player_pane_;
  ConnectionStatusWidget* connection_status_;
  QLabel* status_label_;
  QStackedWidget* content_stack_;
  QPushButton* nav_library_;
  QPushButton* nav_history_;
  QPushButton* nav_search_;
  PlaylistStore* playlist_store_;
  PlaylistPane* playlist_pane_;
  LibraryPane* library_pane_;
  UploadController* upload_;
  PlayHistoryStore* history_store_;
  HistoryPane* history_pane_;

  CatalogModel* model_;
  CatalogClient* catalog_;
  PushClient* push_;
  PlaybackController* playback_;
  DownloadService* downloads_;
  QString current_query_;
  Video current_video_;
  bool playing_from_playlist_;
  qint64 last_history_save_ms_;
};

}  // namespace client
}  // namespace video_player
