#include "square_dance/client/api/push_json.h"

#include <QFile>
#include <gtest/gtest.h>

namespace {

QByteArray LoadFixture(const char* name) {
#ifndef SQUARE_DANCE_SOURCE_DIR
#error SQUARE_DANCE_SOURCE_DIR must be defined by CMake
#endif
  const QString path =
      QString::fromUtf8(SQUARE_DANCE_SOURCE_DIR) +
      QString::fromUtf8("/tests/client/api/fixtures/") +
      QString::fromUtf8(name);
  QFile file(path);
  EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << path.toStdString();
  return file.readAll();
}

}  // namespace

TEST(PushJson, ParsesUpsertedFixture) {
  square_dance::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(square_dance::client::ParsePushMessage(
      LoadFixture("push_upserted.json"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, square_dance::client::PushMessage::kUpserted);
  EXPECT_EQ(msg.video.id, 3);
  EXPECT_EQ(msg.video.title, QString::fromUtf8("新编广场舞"));
}

TEST(PushJson, ParsesRemovedFixture) {
  square_dance::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(square_dance::client::ParsePushMessage(
      LoadFixture("push_removed.json"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, square_dance::client::PushMessage::kRemoved);
  EXPECT_EQ(msg.removed_id, 3);
}

TEST(PushJson, IgnoresUnknownType) {
  square_dance::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(square_dance::client::ParsePushMessage(
      QByteArray("{\"type\":\"ping\",\"nonce\":1}"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, square_dance::client::PushMessage::kIgnored);
}

TEST(PushJson, FailsOnMalformedJson) {
  square_dance::client::PushMessage msg;
  QString error;
  EXPECT_FALSE(square_dance::client::ParsePushMessage(
      QByteArray("{not-json"), &msg, &error));
  EXPECT_FALSE(error.isEmpty());
}
