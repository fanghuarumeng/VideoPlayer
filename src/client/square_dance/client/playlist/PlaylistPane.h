#pragma once

#include "square_dance/client/playlist/PlaylistStore.h"

#include <QHash>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;

namespace square_dance {
namespace client {

class PlaylistPane : public QWidget {
  Q_OBJECT

 public:
  explicit PlaylistPane(PlaylistStore* store, QWidget* parent = 0);

  bool selectedItem(PlaylistItem* out) const;
  void selectRow(int row);
  void refresh();

 signals:
  void playRequested(const PlaylistItem& item);
  void cleared();

 private slots:
  void onClearClicked();
  void onItemDoubleClicked();
  void onThumbFinished();

 private:
  void RebuildList();
  void RequestThumbnails();
  void RequestOneThumbnail(const PlaylistItem& item);
  void CancelAllThumbReplies();
  QListWidgetItem* FindItemById(qint64 id) const;
  PlaylistItem ItemFromRow(int row) const;

  PlaylistStore* store_;
  QListWidget* list_;
  QPushButton* clear_btn_;
  QNetworkAccessManager* nam_;
  QHash<QNetworkReply*, qint64> thumb_replies_;
};

}  // namespace client
}  // namespace square_dance
