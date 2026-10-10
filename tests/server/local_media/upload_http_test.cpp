#include "square_dance/server/api/HttpApiServer.h"
#include "square_dance/server/local_media/LocalMediaStore.h"
#include "square_dance/server/local_media/UploadGate.h"
#include "square_dance/server/video_store/SqliteVideoRepository.h"

#include <gtest/gtest.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace {

void RemoveTreeFile(const std::string& path) {
  std::remove(path.c_str());
}

}  // namespace

class UploadHttpTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = "upload_http_test.db";
    media_root_ = "upload_http_media";
    RemoveTreeFile(db_path_);
#if defined(_WIN32)
    _mkdir(media_root_.c_str());
#else
    mkdir(media_root_.c_str(), 0755);
#endif

    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
    store_.reset(new square_dance::server::LocalMediaStore(&repo_, media_root_));
    gate_.reset(new square_dance::server::UploadGate(1));
    server_.reset(
        new square_dance::server::HttpApiServer(&repo_, NULL));
    server_->set_local_media(store_.get(), gate_.get());
    ASSERT_TRUE(server_->Start("127.0.0.1", 0, &error)) << error;
    ASSERT_GT(server_->bound_port(), 0);
  }

  void TearDown() override {
    if (server_.get() != NULL) {
      server_->Stop();
      server_.reset();
    }
    store_.reset();
    gate_.reset();
    repo_.Close();
    RemoveTreeFile(db_path_);
  }

  std::string db_path_;
  std::string media_root_;
  square_dance::server::SqliteVideoRepository repo_;
  std::unique_ptr<square_dance::server::LocalMediaStore> store_;
  std::unique_ptr<square_dance::server::UploadGate> gate_;
  std::unique_ptr<square_dance::server::HttpApiServer> server_;
};

TEST_F(UploadHttpTest, UploadStoresAndServesMedia) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  client.set_connection_timeout(2, 0);
  client.set_read_timeout(5, 0);

  httplib::MultipartFormDataItems items;
  items.push_back(
      {"file", "fake-video-bytes", "clip.mxf", "application/mxf"});
  items.push_back({"title", "My MXF", "", ""});

  const httplib::Result res =
      client.Post("/api/v1/videos/upload", items);
  ASSERT_TRUE(res) << "upload request failed";
  EXPECT_EQ(res->status, 201);

  const nlohmann::json body = nlohmann::json::parse(res->body);
  EXPECT_EQ(body.at("source_type").get<std::string>(), "local_upload");
  EXPECT_EQ(body.at("title").get<std::string>(), "My MXF");
  EXPECT_TRUE(body.at("thumbnail_url").get<std::string>().empty());
  const std::string play_url = body.at("play_url").get<std::string>();
  ASSERT_FALSE(play_url.empty());

  const std::string prefix =
      "http://127.0.0.1:" + std::to_string(server_->bound_port());
  ASSERT_EQ(play_url.substr(0, prefix.size()), prefix);

  const std::string media_path = play_url.substr(prefix.size());
  const httplib::Result media = client.Get(media_path.c_str());
  ASSERT_TRUE(media);
  EXPECT_EQ(media->status, 200);
  EXPECT_EQ(media->body, "fake-video-bytes");
}

TEST_F(UploadHttpTest, RejectsDisallowedExtension) {
  httplib::Client client("127.0.0.1", server_->bound_port());
  httplib::MultipartFormDataItems items;
  items.push_back({"file", "x", "notes.txt", "text/plain"});
  const httplib::Result res = client.Post("/api/v1/videos/upload", items);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 400);
}

TEST_F(UploadHttpTest, ReturnsBusyWhenGateFull) {
  ASSERT_TRUE(gate_->tryAcquire());
  httplib::Client client("127.0.0.1", server_->bound_port());
  httplib::MultipartFormDataItems items;
  items.push_back({"file", "bytes", "a.mp4", "video/mp4"});
  const httplib::Result res = client.Post("/api/v1/videos/upload", items);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 503);
  const nlohmann::json body = nlohmann::json::parse(res->body);
  EXPECT_EQ(body.at("error").get<std::string>(), "upload_busy");
  gate_->release();
}
