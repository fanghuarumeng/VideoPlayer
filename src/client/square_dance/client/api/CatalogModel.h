#pragma once

#include "square_dance/client/api/Video.h"

#include <QAbstractListModel>
#include <QVector>

namespace square_dance {
namespace client {

class CatalogModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    ExternalIdRole,
    TitleRole,
    PlayUrlRole,
    DownloadUrlRole,
    SourceTypeRole,
    UpdatedAtRole
  };

  explicit CatalogModel(QObject* parent = 0);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QHash<int, QByteArray> roleNames() const;

  Video videoAt(int row) const;

 public slots:
  void resetFromSearch(const QVector<square_dance::client::Video>& videos);

 private:
  QVector<Video> videos_;
};

}  // namespace client
}  // namespace square_dance
