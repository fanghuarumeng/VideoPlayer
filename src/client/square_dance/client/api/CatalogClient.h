#pragma once

#include "square_dance/client/api/Video.h"

#include <QObject>
#include <QString>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

namespace square_dance {
namespace client {

class CatalogClient : public QObject {
  Q_OBJECT

 public:
  explicit CatalogClient(const QString& http_base_url, QObject* parent = 0);

  void search(const QString& name, int limit = 20);
  void getById(qint64 id);

  const QString& http_base_url() const;

 signals:
  void searchFinished(const QVector<square_dance::client::Video>& videos);
  void searchFailed(const QString& error);
  void detailFinished(const square_dance::client::Video& video);
  void detailFailed(const QString& error);

 private slots:
  void onSearchFinished();
  void onDetailFinished();

 private:
  CatalogClient(const CatalogClient&);
  CatalogClient& operator=(const CatalogClient&);

  QString NormalizeBaseUrl(const QString& url) const;

  QString http_base_url_;
  QNetworkAccessManager* nam_;
};

}  // namespace client
}  // namespace square_dance
