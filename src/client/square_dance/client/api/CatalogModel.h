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
  QString filterQuery() const;
  int indexOfId(qint64 id) const;

 public slots:
  void resetFromSearch(const QVector<square_dance::client::Video>& videos);
  void setFilterQuery(const QString& query);
  // Elder-ui push rules: empty filter → prepend/update; else keep only title match.
  void applyUpserted(const square_dance::client::Video& video);
  void applyRemoved(qint64 id);

 private:
  bool MatchesFilter(const Video& video) const;

  QVector<Video> videos_;
  QString filter_query_;
};

}  // namespace client
}  // namespace square_dance
