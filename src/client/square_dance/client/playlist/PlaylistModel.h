#pragma once

#include "square_dance/client/playlist/PlaylistStore.h"

#include <QAbstractListModel>

namespace square_dance {
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
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual QHash<int, QByteArray> roleNames() const;

  PlaylistStore* store() const;
  void reload();
  PlaylistItem itemAt(int row) const;

 public slots:
  void refresh();

 private:
  PlaylistStore* store_;
};

}  // namespace client
}  // namespace square_dance
