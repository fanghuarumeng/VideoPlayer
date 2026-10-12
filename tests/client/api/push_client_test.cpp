#include "video_player/client/api/CatalogModel.h"
#include "video_player/client/api/PushClient.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <gtest/gtest.h>

namespace {

bool WaitForSpy(QSignalSpy* spy, int timeout_ms = 3000) {
  if (spy->count() > 0) {
    return true;
  }
  return spy->wait(timeout_ms);
}

}  // namespace

TEST(PushClient, ReceivesUpsertAndUpdatesModel) {
  QWebSocketServer server(QString::fromUtf8("push-test"),
                          QWebSocketServer::NonSecureMode);
  ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

  QWebSocket* peer = NULL;
  QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
    peer = server.nextPendingConnection();
  });

  video_player::client::PushClient client;
  video_player::client::CatalogModel model;
  model.setFilterQuery(QString());
  QObject::connect(&client, &video_player::client::PushClient::videoUpserted,
                   &model, &video_player::client::CatalogModel::applyUpserted);
  QObject::connect(&client, &video_player::client::PushClient::videoRemoved,
                   &model, &video_player::client::CatalogModel::applyRemoved);

  QSignalSpy connected_spy(&client,
                           &video_player::client::PushClient::connectionChanged);
  QSignalSpy upsert_spy(&client,
                        &video_player::client::PushClient::videoUpserted);

  const QUrl url(QString::fromUtf8("ws://127.0.0.1:") +
                 QString::number(server.serverPort()) +
                 QString::fromUtf8("/ws"));
  client.connectToServer(url);
  ASSERT_TRUE(WaitForSpy(&connected_spy));
  ASSERT_TRUE(connected_spy.at(0).at(0).toBool());
  ASSERT_TRUE(peer != NULL);

  const QByteArray payload =
      "{"
      "\"type\":\"video.upserted\","
      "\"video\":{"
      "\"id\":42,\"external_id\":\"e42\",\"title\":\"推送曲\","
      "\"play_url\":\"https://example.com/42.mp4\","
      "\"download_url\":\"https://example.com/42.mp4\","
      "\"source_type\":\"authorized\","
      "\"updated_at\":\"2026-10-09T11:00:00Z\""
      "}}";
  peer->sendTextMessage(QString::fromUtf8(payload));
  ASSERT_TRUE(WaitForSpy(&upsert_spy));
  ASSERT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.videoAt(0).id, 42);
  EXPECT_EQ(model.videoAt(0).title, QString::fromUtf8("推送曲"));

  QSignalSpy removed_spy(&client,
                         &video_player::client::PushClient::videoRemoved);
  peer->sendTextMessage(QString::fromUtf8("{\"type\":\"video.removed\",\"id\":42}"));
  ASSERT_TRUE(WaitForSpy(&removed_spy));
  EXPECT_EQ(model.rowCount(), 0);

  peer->sendTextMessage(QString::fromUtf8("{\"type\":\"noop\"}"));
  // Unknown types ignored — model unchanged.
  EXPECT_EQ(model.rowCount(), 0);

  client.disconnectFromServer();
}

TEST(PushClient, IgnoresUnknownWithoutSignals) {
  QWebSocketServer server(QString::fromUtf8("push-ignore"),
                          QWebSocketServer::NonSecureMode);
  ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

  QWebSocket* peer = NULL;
  QObject::connect(&server, &QWebSocketServer::newConnection, [&]() {
    peer = server.nextPendingConnection();
  });

  video_player::client::PushClient client;
  QSignalSpy upsert_spy(&client,
                        &video_player::client::PushClient::videoUpserted);
  QSignalSpy removed_spy(&client,
                         &video_player::client::PushClient::videoRemoved);
  QSignalSpy connected_spy(&client,
                           &video_player::client::PushClient::connectionChanged);

  client.connectToServer(QUrl(QString::fromUtf8("ws://127.0.0.1:") +
                              QString::number(server.serverPort())));
  ASSERT_TRUE(WaitForSpy(&connected_spy));
  ASSERT_TRUE(peer != NULL);
  peer->sendTextMessage(QString::fromUtf8("{\"type\":\"heartbeat\"}"));
  // Give the event loop a brief turn; no video signals expected.
  QCoreApplication::processEvents();
  EXPECT_EQ(upsert_spy.count(), 0);
  EXPECT_EQ(removed_spy.count(), 0);
  client.disconnectFromServer();
}
