#include "square_dance/server/api/video_json.h"

#include <gtest/gtest.h>

TEST(ServerVideoJson, SerializesApiFields) {
  square_dance::server::Video video;
  video.id = 7;
  video.external_id = "e1";
  video.title = "T";
  video.play_url = "https://example.com/a.mp4";
  video.download_url = "https://example.com/a.mp4";
  video.source_type = "authorized";
  video.consecutive_failures = 2;
  video.created_at = "2026-01-01T00:00:00Z";
  video.updated_at = "2026-01-02T00:00:00Z";

  const nlohmann::json j = square_dance::server::VideoToApiJson(video);
  EXPECT_EQ(j.at("id").get<std::int64_t>(), 7);
  EXPECT_EQ(j.at("external_id").get<std::string>(), "e1");
  EXPECT_EQ(j.at("updated_at").get<std::string>(), "2026-01-02T00:00:00Z");
  EXPECT_FALSE(j.contains("consecutive_failures"));
  EXPECT_FALSE(j.contains("created_at"));
}
