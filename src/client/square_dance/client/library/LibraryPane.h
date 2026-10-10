#pragma once

#include "square_dance/client/api/Video.h"

#include <QHash>
#include <QWidget>
#include <QVector>

class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QLabel;

namespace square_dance {
namespace client {

class LibraryPane : public QWidget {
  Q_OBJECT

 public:
  explicit LibraryPane(QWidget* parent = 0);

  void setVideos(const QVector<Video>& videos);
  void upsertVideo(const Video& video);
  void removeVideo(qint64 id);
  QVector<Video> selectedVideos() const;
  void clearSelection();

 signals:
  void refreshRequested();
  void uploadRequested();
  void addToPlaylistRequested(const QVector<Video>& videos);
  void playRequested(const Video& video);

 private slots:
  void onAddClicked();
  void onItemDoubleClicked();
  void onThumbFinished();

 private:
  void RebuildList();
  void RequestThumbnails();
  void RequestOneThumbnail(const Video& video);
  void CancelAllThumbReplies();
  void CancelThumbRepliesForId(qint64 id);
  QIcon PlaceholderIcon() const;
  QListWidgetItem* FindItemById(qint64 id) const;
  QListWidgetItem* MakeItem(const Video& video) const;

  QListWidget* list_;
  QPushButton* refresh_btn_;
  QPushButton* upload_btn_;
  QPushButton* add_btn_;
  QLabel* empty_label_;
  QVector<Video> videos_;
  QNetworkAccessManager* nam_;
  QHash<QNetworkReply*, qint64> thumb_replies_;
};

}  // namespace client
}  // namespace square_dance
