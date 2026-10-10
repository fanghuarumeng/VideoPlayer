#pragma once

#include "square_dance/client/ClientConfig.h"
#include "square_dance/client/api/Video.h"

#include <QMainWindow>
#include <QString>
#include <QVector>

class QLabel;

namespace square_dance {
namespace client {

class CatalogClient;
class CatalogModel;
class ConnectionStatusWidget;
class DownloadService;
class PlaybackController;
class PlayerPane;
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
  void onPlayClicked();
  void onPauseClicked();
  void onStopClicked();
  void onDownloadClicked();
  void onPlaybackError(const QString& message);
  void onDownloadProgress(qint64 video_id, qint64 received, qint64 total);
  void onDownloadFinished(qint64 video_id, const QString& local_path);
  void onDownloadError(qint64 video_id, const QString& message);
  void onConnectionChanged(bool connected);
  void SetStatusMessage(const QString& message);

 private:
  bool SelectedVideo(Video* out) const;

  ClientConfig config_;
  SearchBar* search_bar_;
  VideoResultList* result_list_;
  PlayerPane* player_pane_;
  ConnectionStatusWidget* connection_status_;
  QLabel* status_label_;

  CatalogModel* model_;
  CatalogClient* catalog_;
  PushClient* push_;
  PlaybackController* playback_;
  DownloadService* downloads_;
  QString current_query_;
};

}  // namespace client
}  // namespace square_dance
