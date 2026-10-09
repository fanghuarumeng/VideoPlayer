#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"

#include <QByteArray>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <gtest/gtest.h>

namespace {

class FakeSearchServer {
 public:
  bool Start(const QByteArray& body, int status = 200) {
    body_ = body;
    status_ = status;
    buffer_.clear();
    server_.reset(new QTcpServer());
    if (!server_->listen(QHostAddress::LocalHost, 0)) {
      return false;
    }
    QObject::connect(server_.data(), &QTcpServer::newConnection, [this]() {
      QTcpSocket* sock = server_->nextPendingConnection();
      if (sock == NULL) {
        return;
      }
      QObject::connect(sock, &QTcpSocket::readyRead, [this, sock]() {
        buffer_.append(sock->readAll());
        if (!buffer_.contains("\r\n\r\n")) {
          return;
        }
        const QByteArray reason = (status_ >= 200 && status_ < 300)
                                      ? QByteArray("OK")
                                      : QByteArray("Error");
        const QByteArray response =
            QByteArray("HTTP/1.1 ") + QByteArray::number(status_) + " " +
            reason +
            "\r\nContent-Type: application/json\r\nContent-Length: " +
            QByteArray::number(body_.size()) + "\r\nConnection: close\r\n\r\n" +
            body_;
        sock->write(response);
        sock->disconnectFromHost();
        buffer_.clear();
      });
    });
    return true;
  }

  quint16 port() const { return server_->serverPort(); }

 private:
  QScopedPointer<QTcpServer> server_;
  QByteArray body_;
  QByteArray buffer_;
  int status_;
};

bool WaitForSpy(QSignalSpy* spy, int timeout_ms = 3000) {
  if (spy->count() > 0) {
    return true;
  }
  return spy->wait(timeout_ms);
}

}  // namespace

TEST(CatalogClient, SearchEmitsFinishedAndFillsModel) {
  const QByteArray body =
      "["
      "{\"id\":7,\"external_id\":\"e7\",\"title\":\"七步舞\","
      "\"play_url\":\"https://example.com/7.mp4\","
      "\"download_url\":\"https://example.com/7.mp4\","
      "\"source_type\":\"authorized\","
      "\"updated_at\":\"2026-10-09T08:00:00Z\"}"
      "]";

  FakeSearchServer fake;
  ASSERT_TRUE(fake.Start(body));

  const QString base =
      QString::fromUtf8("http://127.0.0.1:") + QString::number(fake.port());
  square_dance::client::CatalogClient client(base);
  square_dance::client::CatalogModel model;
  QObject::connect(&client, &square_dance::client::CatalogClient::searchFinished,
                   &model, &square_dance::client::CatalogModel::resetFromSearch);

  QSignalSpy finished_spy(&client,
                          &square_dance::client::CatalogClient::searchFinished);
  QSignalSpy failed_spy(&client,
                        &square_dance::client::CatalogClient::searchFailed);

  client.search(QString::fromUtf8("七"), 20);
  ASSERT_TRUE(WaitForSpy(&finished_spy)) << "searchFinished not emitted";
  EXPECT_EQ(failed_spy.count(), 0);
  ASSERT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.videoAt(0).id, 7);
  EXPECT_EQ(model.videoAt(0).title, QString::fromUtf8("七步舞"));
}

TEST(CatalogClient, SearchEmitsFailedOnHttpError) {
  FakeSearchServer fake;
  ASSERT_TRUE(fake.Start(QByteArray("{\"error\":\"boom\"}"), 500));

  const QString base =
      QString::fromUtf8("http://127.0.0.1:") + QString::number(fake.port());
  square_dance::client::CatalogClient client(base);
  QSignalSpy failed_spy(&client,
                        &square_dance::client::CatalogClient::searchFailed);
  QSignalSpy finished_spy(&client,
                          &square_dance::client::CatalogClient::searchFinished);

  client.search(QString::fromUtf8("x"), 10);
  ASSERT_TRUE(WaitForSpy(&failed_spy)) << "searchFailed not emitted";
  EXPECT_EQ(finished_spy.count(), 0);
  EXPECT_FALSE(failed_spy.at(0).at(0).toString().isEmpty());
}
