#include "square_dance/client/playlist/PlaylistModel.h"

#include "square_dance/client/elder_ui/AppIcons.h"

namespace square_dance {
namespace client {

PlaylistModel::PlaylistModel(PlaylistStore* store, QObject* parent)
    : QAbstractListModel(parent), store_(store) {}

PlaylistStore* PlaylistModel::store() const {
  return store_;
}

int PlaylistModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid() || store_ == NULL) {
    return 0;
  }
  return store_->items().size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || store_ == NULL) {
    return QVariant();
  }
  const QVector<PlaylistItem> items = store_->items();
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const PlaylistItem& item = items[index.row()];
  switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
      return item.title;
    case Qt::DecorationRole:
      return AppIcons::Film();
    case IdRole:
      return static_cast<qulonglong>(item.id);
    case PlayUrlRole:
      return item.play_url;
    case ThumbnailUrlRole:
      return item.thumbnail_url;
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[IdRole] = "id";
  roles[TitleRole] = "title";
  roles[PlayUrlRole] = "play_url";
  roles[ThumbnailUrlRole] = "thumbnail_url";
  return roles;
}

void PlaylistModel::reload() {
  beginResetModel();
  endResetModel();
}

void PlaylistModel::refresh() {
  reload();
}

PlaylistItem PlaylistModel::itemAt(int row) const {
  if (store_ == NULL) {
    return PlaylistItem();
  }
  const QVector<PlaylistItem> items = store_->items();
  if (row < 0 || row >= items.size()) {
    return PlaylistItem();
  }
  return items[row];
}

}  // namespace client
}  // namespace square_dance
