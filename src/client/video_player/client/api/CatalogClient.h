#pragma once

#include "video_player/client/api/Video.h"

#include <QObject>
#include <QString>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

namespace video_player {
namespace client {

class CatalogClient : public QObject {
  Q_OBJECT

 public:
  explicit CatalogClient(const QString& http_base_url, QObject* parent = 0);

  void search(const QString& name, int limit = 20);
  void listVideos(int limit = 50, int offset = 0);
  void getById(qint64 id);

  const QString& http_base_url() const;

 signals:
  void searchFinished(const QVector<video_player::client::Video>& videos);
  void searchFailed(const QString& error);
  void listFinished(const QVector<video_player::client::Video>& videos);
  void listFailed(const QString& error);
  void detailFinished(const video_player::client::Video& video);
  void detailFailed(const QString& error);

 private slots:
  void onSearchFinished();
  void onListFinished();
  void onDetailFinished();

 private:
  CatalogClient(const CatalogClient&);
  CatalogClient& operator=(const CatalogClient&);

  QString NormalizeBaseUrl(const QString& url) const;

  QString http_base_url_;
  QNetworkAccessManager* nam_;
};

}  // namespace client
}  // namespace video_player
