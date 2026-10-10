#include "square_dance/server/api/HttpApiServer.h"
#include "square_dance/server/video_store/SqliteVideoRepository.h"
#include "square_dance/server/video_store/Video.h"

#include <gtest/gtest.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

namespace {

square_dance::server::Video MakeVideo(const std::string& id,
                                      const std::string& title) {
  square_dance::server::Video v;
  v.id = 0;
  v.external_id = id;
  v.title = title;
  v.play_url = "https://example.com/" + id + ".mp4";
  v.download_url = v.play_url;
  v.source_type = "authorized";
  v.consecutive_failures = 0;
  v.created_at = "2026-10-09T00:00:00Z";
  v.updated_at = "2026-10-09T00:00:00Z";
  return v;
}

}  // namespace

class HttpSearchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = "http_search_test.db";
    std::remove(db_path_.c_str());
    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
    ASSERT_TRUE(repo_.upsert(MakeVideo("1", "Square Dance Morning"), &error))
        << error;
    ASSERT_TRUE(repo_.upsert(MakeVideo("2", "Other Clip"), &error)) << error;

    server_.reset(new square_dance::server::HttpApiServer(&repo_, NULL));
    ASSERT_TRUE(server_->Start("127.0.0.1", 0, &error)) << error;
    ASSERT_GT(server_->bound_port(), 0);
  }

  void TearDown() override {
    if (server_.get() != NULL) {
      server_->Stop();
      server_.reset();
    }
    repo_.Close();
    std::remove(db_path_.c_str());
  }

  std::string db_path_;
  square_dance::server::SqliteVideoRepository repo_;
  std::unique_ptr<square_dance::server::HttpApiServer> server_;
};

TEST_F(HttpSearchTest, SearchReturnsMatchingVideos) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  client.set_connection_timeout(2, 0);
  client.set_read_timeout(2, 0);

  const httplib::Result res =
      client.Get("/api/v1/videos/search?q=square&limit=10");
  ASSERT_TRUE(res) << "HTTP request failed";
  EXPECT_EQ(res->status, 200);

  const nlohmann::json body = nlohmann::json::parse(res->body);
  ASSERT_TRUE(body.is_array());
  ASSERT_EQ(body.size(), 1u);
  EXPECT_EQ(body[0].at("external_id").get<std::string>(), "1");
  EXPECT_EQ(body[0].at("title").get<std::string>(), "Square Dance Morning");
}

TEST_F(HttpSearchTest, GetByIdAndMissing) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  const std::vector<square_dance::server::Video> all = repo_.listAll(10, 0);
  ASSERT_FALSE(all.empty());

  std::ostringstream path;
  path << "/api/v1/videos/" << all[0].id;
  httplib::Result ok = client.Get(path.str().c_str());
  ASSERT_TRUE(ok);
  EXPECT_EQ(ok->status, 200);

  httplib::Result missing = client.Get("/api/v1/videos/999999");
  ASSERT_TRUE(missing);
  EXPECT_EQ(missing->status, 404);
}

TEST_F(HttpSearchTest, IngestUnavailableWithoutService) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  httplib::Result res = client.Post("/api/v1/ingest/run");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 503);
}

TEST_F(HttpSearchTest, ListVideosNewestFirstIncludesThumbnailUrl) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  client.set_connection_timeout(2, 0);
  client.set_read_timeout(2, 0);

  const httplib::Result res = client.Get("/api/v1/videos?limit=50&offset=0");
  ASSERT_TRUE(res) << "HTTP request failed";
  EXPECT_EQ(res->status, 200);

  const nlohmann::json body = nlohmann::json::parse(res->body);
  ASSERT_TRUE(body.is_array());
  ASSERT_EQ(body.size(), 2u);
  EXPECT_EQ(body[0].at("external_id").get<std::string>(), "2");
  EXPECT_EQ(body[1].at("external_id").get<std::string>(), "1");
  EXPECT_TRUE(body[0].contains("thumbnail_url"));
  EXPECT_EQ(body[0].at("thumbnail_url").get<std::string>(), "");
}
