#include "square_dance/client/api/CatalogClient.h"

#include "square_dance/client/api/catalog_json.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace square_dance {
namespace client {

CatalogClient::CatalogClient(const QString& http_base_url, QObject* parent)
    : QObject(parent),
      http_base_url_(NormalizeBaseUrl(http_base_url)),
      nam_(new QNetworkAccessManager(this)) {
  qRegisterMetaType<square_dance::client::Video>(
      "square_dance::client::Video");
  qRegisterMetaType<QVector<square_dance::client::Video> >(
      "QVector<square_dance::client::Video>");
}

const QString& CatalogClient::http_base_url() const { return http_base_url_; }

QString CatalogClient::NormalizeBaseUrl(const QString& url) const {
  QString out = url.trimmed();
  while (out.endsWith(QLatin1Char('/'))) {
    out.chop(1);
  }
  return out;
}

void CatalogClient::search(const QString& name, int limit) {
  if (limit <= 0) {
    limit = 20;
  }
  QUrl url(http_base_url_ + QString::fromUtf8("/api/v1/videos/search"));
  QUrlQuery query;
  query.addQueryItem(QString::fromUtf8("q"), name);
  query.addQueryItem(QString::fromUtf8("limit"), QString::number(limit));
  url.setQuery(query);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    QString::fromUtf8("application/json"));
  QNetworkReply* reply = nam_->get(request);
  connect(reply, &QNetworkReply::finished, this,
          &CatalogClient::onSearchFinished);
}

void CatalogClient::listVideos(int limit, int offset) {
  if (limit <= 0) {
    limit = 50;
  }
  if (limit > 200) {
    limit = 200;
  }
  if (offset < 0) {
    offset = 0;
  }
  QUrl url(http_base_url_ + QString::fromUtf8("/api/v1/videos"));
  QUrlQuery query;
  query.addQueryItem(QString::fromUtf8("limit"), QString::number(limit));
  query.addQueryItem(QString::fromUtf8("offset"), QString::number(offset));
  url.setQuery(query);

  QNetworkRequest request(url);
  QNetworkReply* reply = nam_->get(request);
  connect(reply, &QNetworkReply::finished, this,
          &CatalogClient::onListFinished);
}

void CatalogClient::getById(qint64 id) {
  const QUrl url(http_base_url_ + QString::fromUtf8("/api/v1/videos/") +
                 QString::number(id));
  QNetworkRequest request(url);
  QNetworkReply* reply = nam_->get(request);
  connect(reply, &QNetworkReply::finished, this,
          &CatalogClient::onDetailFinished);
}

void CatalogClient::onSearchFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    emit searchFailed(QString::fromUtf8("internal: missing reply"));
    return;
  }
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    emit searchFailed(reply->errorString());
    return;
  }

  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray body = reply->readAll();
  if (status < 200 || status >= 300) {
    emit searchFailed(QString::fromUtf8("HTTP ") + QString::number(status) +
                      QString::fromUtf8(": ") + QString::fromUtf8(body));
    return;
  }

  QVector<Video> videos;
  QString error;
  if (!ParseVideoListJson(body, &videos, &error)) {
    emit searchFailed(error);
    return;
  }
  emit searchFinished(videos);
}

void CatalogClient::onListFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    emit listFailed(QString::fromUtf8("internal: missing reply"));
    return;
  }
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    emit listFailed(reply->errorString());
    return;
  }

  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray body = reply->readAll();
  if (status < 200 || status >= 300) {
    emit listFailed(QString::fromUtf8("HTTP ") + QString::number(status) +
                    QString::fromUtf8(": ") + QString::fromUtf8(body));
    return;
  }

  QVector<Video> videos;
  QString error;
  if (!ParseVideoListJson(body, &videos, &error)) {
    emit listFailed(error);
    return;
  }
  emit listFinished(videos);
}

void CatalogClient::onDetailFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    emit detailFailed(QString::fromUtf8("internal: missing reply"));
    return;
  }
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    emit detailFailed(reply->errorString());
    return;
  }

  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray body = reply->readAll();
  if (status < 200 || status >= 300) {
    emit detailFailed(QString::fromUtf8("HTTP ") + QString::number(status) +
                      QString::fromUtf8(": ") + QString::fromUtf8(body));
    return;
  }

  Video video;
  QString error;
  if (!ParseVideoJson(body, &video, &error)) {
    emit detailFailed(error);
    return;
  }
  emit detailFinished(video);
}

}  // namespace client
}  // namespace square_dance
