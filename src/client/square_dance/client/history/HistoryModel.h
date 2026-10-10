#pragma once

#include "square_dance/client/history/PlayHistoryStore.h"

#include <QAbstractListModel>

namespace square_dance {
namespace client {

class HistoryModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    PlayUrlRole,
    ThumbnailUrlRole,
    PositionMsRole,
    DurationMsRole,
    ResumeMsRole,
    ProgressTextRole
  };

  explicit HistoryModel(PlayHistoryStore* store, QObject* parent = 0);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QHash<int, QByteArray> roleNames() const;

  PlayHistoryItem itemAt(int row) const;

 public slots:
  void reload();

 private:
  static QString FormatTime(qint64 ms);

  PlayHistoryStore* store_;
};

}  // namespace client
}  // namespace square_dance
