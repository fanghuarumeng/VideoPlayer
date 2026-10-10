#pragma once

#include "square_dance/client/api/Video.h"

#include <QWidget>
#include <QVector>

class QListWidget;
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

 private:
  void RebuildList();
  QIcon PlaceholderIcon() const;

  QListWidget* list_;
  QPushButton* refresh_btn_;
  QPushButton* upload_btn_;
  QPushButton* add_btn_;
  QLabel* empty_label_;
  QVector<Video> videos_;
};

}  // namespace client
}  // namespace square_dance
