#include "video_player/client/api/push_json.h"

#include <QFile>
#include <gtest/gtest.h>

namespace {

QByteArray LoadFixture(const char* name) {
#ifndef VIDEO_PLAYER_SOURCE_DIR
#error VIDEO_PLAYER_SOURCE_DIR must be defined by CMake
#endif
  const QString path =
      QString::fromUtf8(VIDEO_PLAYER_SOURCE_DIR) +
      QString::fromUtf8("/tests/client/api/fixtures/") +
      QString::fromUtf8(name);
  QFile file(path);
  EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << path.toStdString();
  return file.readAll();
}

}  // namespace

TEST(PushJson, ParsesUpsertedFixture) {
  video_player::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(video_player::client::ParsePushMessage(
      LoadFixture("push_upserted.json"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, video_player::client::PushMessage::kUpserted);
  EXPECT_EQ(msg.video.id, 3);
  EXPECT_EQ(msg.video.title, QString::fromUtf8("新编广场舞"));
}

TEST(PushJson, ParsesRemovedFixture) {
  video_player::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(video_player::client::ParsePushMessage(
      LoadFixture("push_removed.json"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, video_player::client::PushMessage::kRemoved);
  EXPECT_EQ(msg.removed_id, 3);
}

TEST(PushJson, IgnoresUnknownType) {
  video_player::client::PushMessage msg;
  QString error;
  ASSERT_TRUE(video_player::client::ParsePushMessage(
      QByteArray("{\"type\":\"ping\",\"nonce\":1}"), &msg, &error))
      << error.toStdString();
  EXPECT_EQ(msg.kind, video_player::client::PushMessage::kIgnored);
}

TEST(PushJson, FailsOnMalformedJson) {
  video_player::client::PushMessage msg;
  QString error;
  EXPECT_FALSE(video_player::client::ParsePushMessage(
      QByteArray("{not-json"), &msg, &error));
  EXPECT_FALSE(error.isEmpty());
}
