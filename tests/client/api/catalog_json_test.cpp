#include "video_player/client/api/catalog_json.h"

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

TEST(CatalogJson, ParsesSearchResultsFixture) {
  const QByteArray bytes = LoadFixture("search_results.json");
  QVector<video_player::client::Video> videos;
  QString error;
  ASSERT_TRUE(video_player::client::ParseVideoListJson(bytes, &videos, &error))
      << error.toStdString();
  ASSERT_EQ(videos.size(), 2);
  EXPECT_EQ(videos[0].id, 1);
  EXPECT_EQ(videos[0].external_id, QString::fromUtf8("auth-001"));
  EXPECT_EQ(videos[0].title, QString::fromUtf8("广场舞基础步伐"));
  EXPECT_EQ(videos[0].source_type, QString::fromUtf8("authorized"));
  EXPECT_EQ(videos[1].id, 2);
  EXPECT_EQ(videos[1].source_type, QString::fromUtf8("archive_org"));
  EXPECT_TRUE(videos[1].play_url.startsWith(QString::fromUtf8("https://")));
}

TEST(CatalogJson, ParsesSingleVideoObject) {
  const QByteArray bytes = LoadFixture("search_results.json");
  QVector<video_player::client::Video> videos;
  QString error;
  ASSERT_TRUE(video_player::client::ParseVideoListJson(bytes, &videos, &error));
  const QByteArray one = video_player::client::VideoToJsonBytes(videos[0]);
  video_player::client::Video out;
  ASSERT_TRUE(video_player::client::ParseVideoJson(one, &out, &error))
      << error.toStdString();
  EXPECT_EQ(out.id, videos[0].id);
  EXPECT_EQ(out.title, videos[0].title);
}

TEST(CatalogJson, FailsOnNonArrayForList) {
  QVector<video_player::client::Video> videos;
  QString error;
  EXPECT_FALSE(video_player::client::ParseVideoListJson(
      QByteArray("{\"id\":1}"), &videos, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(CatalogJson, FailsOnMissingPlayUrlInList) {
  const QByteArray json =
      "[{\"id\":1,\"external_id\":\"x\",\"title\":\"t\","
      "\"download_url\":\"https://e/a.mp4\",\"source_type\":\"authorized\","
      "\"updated_at\":\"2026-01-01T00:00:00Z\"}]";
  QVector<video_player::client::Video> videos;
  QString error;
  EXPECT_FALSE(video_player::client::ParseVideoListJson(json, &videos, &error));
  EXPECT_FALSE(error.isEmpty());
}
