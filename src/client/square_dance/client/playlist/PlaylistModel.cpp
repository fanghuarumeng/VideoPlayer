#include "square_dance/client/playlist/PlaylistModel.h"

#include "square_dance/client/elder_ui/AppIcons.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSize>
#include <QUrl>

namespace square_dance {
namespace client {

PlaylistModel::PlaylistModel(PlaylistStore* store, QObject* parent)
    : QAbstractListModel(parent),
      store_(store),
      nam_(new QNetworkAccessManager(this)) {}

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
      if (icons_.contains(item.id)) {
        return icons_.value(item.id);
      }
      return AppIcons::Film();
    case Qt::SizeHintRole:
      return QSize(0, 72);
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
  // Copy keys first: abort() may emit finished synchronously and must not
  // mutate thumb_replies_ while iterating it.
  const QList<QNetworkReply*> replies = thumb_replies_.keys();
  thumb_replies_.clear();
  for (int i = 0; i < replies.size(); ++i) {
    QNetworkReply* reply = replies.at(i);
    if (reply == NULL) {
      continue;
    }
    disconnect(reply, 0, this, 0);
    reply->abort();
    reply->deleteLater();
  }
  icons_.clear();

  beginResetModel();
  endResetModel();
  RequestThumbnails();
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

int PlaylistModel::RowForId(qint64 id) const {
  if (store_ == NULL) {
    return -1;
  }
  const QVector<PlaylistItem> items = store_->items();
  for (int i = 0; i < items.size(); ++i) {
    if (items[i].id == id) {
      return i;
    }
  }
  return -1;
}

void PlaylistModel::RequestThumbnails() {
  if (store_ == NULL) {
    return;
  }
  const QVector<PlaylistItem> items = store_->items();
  for (int i = 0; i < items.size(); ++i) {
    const PlaylistItem& item = items[i];
    const QString url = item.thumbnail_url.trimmed();
    if (url.isEmpty() || item.id == 0 || icons_.contains(item.id)) {
      continue;
    }
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    QNetworkReply* reply = nam_->get(req);
    thumb_replies_.insert(reply, item.id);
    connect(reply, &QNetworkReply::finished, this,
            &PlaylistModel::onThumbFinished);
  }
}

void PlaylistModel::onThumbFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    return;
  }
  const qint64 id = thumb_replies_.take(reply);
  reply->deleteLater();
  if (id == 0 || reply->error() != QNetworkReply::NoError) {
    return;
  }
  QPixmap pix;
  if (!pix.loadFromData(reply->readAll())) {
    return;
  }
  icons_.insert(
      id, QIcon(pix.scaled(QSize(96, 54), Qt::KeepAspectRatio,
                           Qt::SmoothTransformation)));
  const int row = RowForId(id);
  if (row < 0) {
    return;
  }
  const QModelIndex idx = index(row, 0);
  emit dataChanged(idx, idx, QVector<int>() << Qt::DecorationRole);
}

}  // namespace client
}  // namespace square_dance
