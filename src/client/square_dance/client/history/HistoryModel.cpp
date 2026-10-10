#include "square_dance/client/history/HistoryModel.h"

namespace square_dance {
namespace client {

HistoryModel::HistoryModel(PlayHistoryStore* store, QObject* parent)
    : QAbstractListModel(parent), store_(store) {}

int HistoryModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid() || store_ == NULL) {
    return 0;
  }
  return store_->items().size();
}

QString HistoryModel::FormatTime(qint64 ms) {
  if (ms < 0) {
    ms = 0;
  }
  const qint64 total_sec = ms / 1000;
  const qint64 m = total_sec / 60;
  const qint64 s = total_sec % 60;
  return QString::fromUtf8("%1:%2")
      .arg(m, 2, 10, QLatin1Char('0'))
      .arg(s, 2, 10, QLatin1Char('0'));
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || store_ == NULL) {
    return QVariant();
  }
  const QVector<PlayHistoryItem> items = store_->items();
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const PlayHistoryItem& h = items.at(index.row());
  const qint64 resume_ms = PlayHistoryStore::ResumePositionMs(h);
  switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
      return h.title;
    case IdRole:
      return QVariant::fromValue(h.id);
    case PlayUrlRole:
      return h.play_url;
    case ThumbnailUrlRole:
      return h.thumbnail_url;
    case PositionMsRole:
      return QVariant::fromValue(h.position_ms);
    case DurationMsRole:
      return QVariant::fromValue(h.duration_ms);
    case ResumeMsRole:
      return QVariant::fromValue(resume_ms);
    case ProgressTextRole:
      return FormatTime(resume_ms) + QString::fromUtf8(" / ") +
             FormatTime(h.duration_ms);
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
  QHash<int, QByteArray> names;
  names.insert(IdRole, "id");
  names.insert(TitleRole, "title");
  names.insert(PlayUrlRole, "play_url");
  names.insert(ThumbnailUrlRole, "thumbnail_url");
  names.insert(PositionMsRole, "position_ms");
  names.insert(DurationMsRole, "duration_ms");
  names.insert(ResumeMsRole, "resume_ms");
  names.insert(ProgressTextRole, "progress_text");
  return names;
}

PlayHistoryItem HistoryModel::itemAt(int row) const {
  if (store_ == NULL) {
    return PlayHistoryItem();
  }
  const QVector<PlayHistoryItem> items = store_->items();
  if (row < 0 || row >= items.size()) {
    return PlayHistoryItem();
  }
  return items.at(row);
}

void HistoryModel::reload() {
  beginResetModel();
  endResetModel();
}

}  // namespace client
}  // namespace square_dance
