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

void CatalogModel::resetFromSearch(
    const QVector<square_dance::client::Video>& videos) {
  beginResetModel();
  videos_ = videos;
  endResetModel();
}

}  // namespace client
}  // namespace square_dance
