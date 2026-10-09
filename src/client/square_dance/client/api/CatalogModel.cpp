#include "square_dance/client/api/CatalogModel.h"

namespace square_dance {
namespace client {

CatalogModel::CatalogModel(QObject* parent) : QAbstractListModel(parent) {}

int CatalogModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return videos_.size();
}

QVariant CatalogModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= videos_.size()) {
    return QVariant();
  }
  const Video& v = videos_.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
      return v.title;
    case IdRole:
      return QVariant::fromValue(v.id);
    case ExternalIdRole:
      return v.external_id;
    case PlayUrlRole:
      return v.play_url;
    case DownloadUrlRole:
      return v.download_url;
    case SourceTypeRole:
      return v.source_type;
    case UpdatedAtRole:
      return v.updated_at;
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> CatalogModel::roleNames() const {
  QHash<int, QByteArray> names;
  names.insert(IdRole, "id");
  names.insert(ExternalIdRole, "external_id");
  names.insert(TitleRole, "title");
  names.insert(PlayUrlRole, "play_url");
  names.insert(DownloadUrlRole, "download_url");
  names.insert(SourceTypeRole, "source_type");
  names.insert(UpdatedAtRole, "updated_at");
  return names;
}

Video CatalogModel::videoAt(int row) const {
  if (row < 0 || row >= videos_.size()) {
    return Video();
  }
  return videos_.at(row);
}

QString CatalogModel::filterQuery() const { return filter_query_; }

int CatalogModel::indexOfId(qint64 id) const {
  for (int i = 0; i < videos_.size(); ++i) {
    if (videos_.at(i).id == id) {
      return i;
    }
  }
  return -1;
}

bool CatalogModel::MatchesFilter(const Video& video) const {
  if (filter_query_.isEmpty()) {
    return true;
  }
  return video.title.contains(filter_query_, Qt::CaseInsensitive);
}

void CatalogModel::resetFromSearch(
    const QVector<square_dance::client::Video>& videos) {
  beginResetModel();
  videos_ = videos;
  endResetModel();
}

void CatalogModel::setFilterQuery(const QString& query) {
  filter_query_ = query;
}

void CatalogModel::applyUpserted(const square_dance::client::Video& video) {
  const int existing = indexOfId(video.id);
  if (!MatchesFilter(video)) {
    if (existing >= 0) {
      beginRemoveRows(QModelIndex(), existing, existing);
      videos_.removeAt(existing);
      endRemoveRows();
    }
    return;
  }

  if (existing >= 0) {
    videos_[existing] = video;
    const QModelIndex idx = index(existing);
    emit dataChanged(idx, idx);
    return;
  }

  beginInsertRows(QModelIndex(), 0, 0);
  videos_.prepend(video);
  endInsertRows();
}

void CatalogModel::applyRemoved(qint64 id) {
  const int existing = indexOfId(id);
  if (existing < 0) {
    return;
  }
  beginRemoveRows(QModelIndex(), existing, existing);
  videos_.removeAt(existing);
  endRemoveRows();
}

}  // namespace client
}  // namespace square_dance
