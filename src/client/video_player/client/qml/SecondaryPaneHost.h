#pragma once

#include "video_player/client/ClientConfig.h"
#include "video_player/client/api/Video.h"

#include <QObject>
#include <QVector>

namespace video_player {
namespace client {

class CatalogClient;
class CatalogModel;
class HistoryPane;
class LibraryPane;
class PlayHistoryStore;
class PlaylistStore;
class SearchBar;
class UploadController;
class VideoResultList;

// Opens library / history / search as modal QWidget dialogs from the QML shell.
class SecondaryPaneHost : public QObject {
  Q_OBJECT

 public:
  SecondaryPaneHost(const ClientConfig& config, CatalogClient* catalog,
                    CatalogModel* catalog_model, PlaylistStore* playlist,
                    PlayHistoryStore* history, UploadController* upload,
                    QObject* parent = 0);

  void setLibraryVideos(const QVector<Video>& videos);
  void upsertLibraryVideo(const Video& video);

 public slots:
  void openLibrary();
  void openHistory();
  void openSearch();

 signals:
  void libraryPlayRequested(const Video& video);
  void historyPlayRequested(const Video& video, qint64 position_ms);
  void statusMessage(const QString& message);
  void playlistChanged();

 private slots:
  void onLibraryRefresh();
  void onLibraryUpload();
  void onLibraryAddToPlaylist(const QVector<Video>& videos);
  void onSearchRequested(const QString& query);
  void onAddSearchToPlaylist();
  void onSearchFinished(const QVector<Video>& videos);

 private:
  ClientConfig config_;
  CatalogClient* catalog_;
  CatalogModel* catalog_model_;
  PlaylistStore* playlist_;
  PlayHistoryStore* history_;
  UploadController* upload_;
  LibraryPane* library_pane_;
  HistoryPane* history_pane_;
  SearchBar* search_bar_;
  VideoResultList* result_list_;
};

}  // namespace client
}  // namespace video_player
