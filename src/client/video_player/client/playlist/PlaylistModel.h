#pragma once

#include "video_player/client/playlist/PlaylistStore.h"

#include <QAbstractListModel>
#include <QHash>
#include <QIcon>
#include <QPixmap>

class QNetworkAccessManager;
class QNetworkReply;

namespace video_player {
namespace client {

class PlaylistModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    PlayUrlRole,
    ThumbnailUrlRole
  };

  explicit PlaylistModel(PlaylistStore* store, QObject* parent = 0);

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const;
  virtual QHash<int, QByteArray> roleNames() const;

  PlaylistStore* store() const;
  void reload();
  PlaylistItem itemAt(int row) const;

 public slots:
  void refresh();

 private slots:
  void onThumbFinished();

 private:
  void RequestThumbnails();
  int RowForId(qint64 id) const;

  PlaylistStore* store_;
  QNetworkAccessManager* nam_;
  QHash<qint64, QIcon> icons_;
  QHash<QNetworkReply*, qint64> thumb_replies_;
};

}  // namespace client
}  // namespace video_player
