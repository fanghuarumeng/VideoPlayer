#include "square_dance/server/ServerConfig.h"

#include <gtest/gtest.h>

#include <string>

namespace {

std::string FixturePath(const char* name) {
#ifndef SQUARE_DANCE_SOURCE_DIR
#error SQUARE_DANCE_SOURCE_DIR must be defined by CMake
#endif
  return std::string(SQUARE_DANCE_SOURCE_DIR) + "/tests/server/fixtures/" +
         name;
}

}  // namespace

TEST(ServerConfig, LoadsValidJsonFixture) {
  square_dance::server::ServerConfig cfg;
  std::string error;
  const std::string path = FixturePath("server_config_valid.json");
  ASSERT_TRUE(square_dance::server::LoadServerConfigFromFile(path, &cfg, &error))
      << error << " path=" << path;

  EXPECT_EQ(cfg.bind_host, "127.0.0.1");
  EXPECT_EQ(cfg.http_port, 8080);
  EXPECT_EQ(cfg.ws_port, 8081);
  EXPECT_EQ(cfg.sqlite_path, "data/videos.db");
  EXPECT_EQ(cfg.ingest_interval_ms, 43200000);
  EXPECT_EQ(cfg.health_interval_ms, 43200000);
  EXPECT_EQ(cfg.url_failure_threshold, 3);
  ASSERT_EQ(cfg.ingest_keywords.size(), 2u);
  EXPECT_EQ(cfg.ingest_keywords[0], "square dance");
  EXPECT_EQ(cfg.ingest_keywords[1], "guangchangwu");
  EXPECT_EQ(cfg.authorized_catalog_path,
            "config/authorized_catalog.example.json");
}

TEST(ServerConfig, AppliesDefaultsWhenOptionalFieldsOmitted) {
  const std::string json = R"({
    "sqlite_path": "x.db",
    "authorized_catalog_path": "cat.json"
  })";
  square_dance::server::ServerConfig cfg;
  std::string error;
  ASSERT_TRUE(square_dance::server::LoadServerConfigFromString(json, &cfg, &error))
      << error;
  EXPECT_EQ(cfg.bind_host, "127.0.0.1");
  EXPECT_EQ(cfg.http_port, 8080);
  EXPECT_EQ(cfg.ws_port, 8081);
  EXPECT_EQ(cfg.ingest_interval_ms, 12 * 60 * 60 * 1000);
  EXPECT_EQ(cfg.health_interval_ms, 12 * 60 * 60 * 1000);
  EXPECT_EQ(cfg.url_failure_threshold, 3);
  ASSERT_EQ(cfg.ingest_keywords.size(), 2u);
  EXPECT_EQ(cfg.ingest_keywords[0], "广场舞");
  EXPECT_EQ(cfg.ingest_keywords[1], "square dance");
  EXPECT_EQ(cfg.media_root, "data/media");
  EXPECT_EQ(cfg.ffmpeg_path, "ffmpeg");
  EXPECT_EQ(cfg.upload_workers, 2);
  EXPECT_EQ(cfg.thumbnail_workers, 1);
}

TEST(ServerConfig, FailsWhenSqlitePathMissing) {
  const std::string json = R"({"authorized_catalog_path":"cat.json"})";
  square_dance::server::ServerConfig cfg;
  std::string error;
  EXPECT_FALSE(
      square_dance::server::LoadServerConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.empty());
}
