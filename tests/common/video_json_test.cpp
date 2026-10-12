#include "video_player/VideoDto.h"
#include "video_player/video_json.h"

#include <gtest/gtest.h>

#include <string>

using video_player::VideoDto;
using video_player::videoDtoFromJson;
using video_player::videoDtoToJson;
using video_player::videoDtoToJsonString;

TEST(VideoJson, RoundTripPreservesApiFields) {
  VideoDto in;
  in.id = 1;
  in.external_id = "WartimeN1943";
  in.title = "square dance sample";
  in.play_url = "https://archive.org/download/WartimeN1943/WartimeN1943.mp4";
  in.download_url = in.play_url;
  in.source_type = "archive_org";
  in.updated_at = "2026-10-09T08:00:00Z";
  in.thumbnail_url = "http://127.0.0.1:8080/media/thumbs/1.jpg";

  const std::string json = videoDtoToJsonString(in);
  VideoDto out;
  std::string error;
  ASSERT_TRUE(videoDtoFromJson(json, &out, &error)) << error;

  EXPECT_EQ(out.id, in.id);
  EXPECT_EQ(out.external_id, in.external_id);
  EXPECT_EQ(out.title, in.title);
  EXPECT_EQ(out.play_url, in.play_url);
  EXPECT_EQ(out.download_url, in.download_url);
  EXPECT_EQ(out.source_type, in.source_type);
  EXPECT_EQ(out.updated_at, in.updated_at);
  EXPECT_EQ(out.thumbnail_url, in.thumbnail_url);
}

TEST(VideoJson, SerializedObjectUsesSpecFieldNames) {
  VideoDto in;
  in.id = 42;
  in.external_id = "auth-001";
  in.title = "t";
  in.play_url = "https://example.com/a.mp4";
  in.download_url = "https://example.com/a.mp4";
  in.source_type = "authorized";
  in.updated_at = "2026-01-01T00:00:00Z";

  const nlohmann::json j = videoDtoToJson(in);
  EXPECT_TRUE(j.contains("id"));
  EXPECT_TRUE(j.contains("external_id"));
  EXPECT_TRUE(j.contains("title"));
  EXPECT_TRUE(j.contains("play_url"));
  EXPECT_TRUE(j.contains("download_url"));
  EXPECT_TRUE(j.contains("source_type"));
  EXPECT_TRUE(j.contains("updated_at"));
  EXPECT_TRUE(j.contains("thumbnail_url"));
  EXPECT_FALSE(j.contains("externalId"));
  EXPECT_FALSE(j.contains("playUrl"));
}

TEST(VideoJson, FromJsonDefaultsMissingThumbnailUrl) {
  const std::string json =
      R"({"id":1,"external_id":"x","title":"t","play_url":"https://e/a.mp4","download_url":"https://e/a.mp4","source_type":"authorized","updated_at":"2026-01-01T00:00:00Z"})";
  VideoDto out;
  std::string error;
  ASSERT_TRUE(videoDtoFromJson(json, &out, &error)) << error;
  EXPECT_TRUE(out.thumbnail_url.empty());
}

TEST(VideoJson, FromJsonFailsOnMissingPlayUrl) {
  const std::string json =
      R"({"id":1,"external_id":"x","title":"t","download_url":"https://e/a.mp4","source_type":"authorized","updated_at":"2026-01-01T00:00:00Z"})";
  VideoDto out;
  std::string error;
  EXPECT_FALSE(videoDtoFromJson(json, &out, &error));
  EXPECT_FALSE(error.empty());
}
