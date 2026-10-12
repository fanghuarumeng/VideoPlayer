#include "video_player/server/url_health/UrlHealthService.h"
#include "video_player/server/video_store/SqliteVideoRepository.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <map>
#include <string>

namespace {

class FakeProber : public video_player::server::IUrlProber {
 public:
  FakeProber() : default_ok_(true) {}

  virtual bool isReachable(const std::string& url, std::string* error) {
    std::map<std::string, bool>::const_iterator it = per_url_.find(url);
    const bool ok = (it == per_url_.end()) ? default_ok_ : it->second;
    if (!ok && error != NULL) {
      *error = "fake unreachable";
    }
    return ok;
  }

  bool default_ok_;
  std::map<std::string, bool> per_url_;
};

video_player::server::Video MakeVideo(const std::string& id,
                                      const std::string& title) {
  video_player::server::Video v;
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

class UrlHealthServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = "url_health_service_test.db";
    std::remove(db_path_.c_str());
    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
    config_ = video_player::server::UrlHealthConfig::MakeDefault();
    config_.interval_ms = 60 * 60 * 1000;
  }

  void TearDown() override {
    repo_.Close();
    std::remove(db_path_.c_str());
  }

  std::string db_path_;
  video_player::server::SqliteVideoRepository repo_;
  video_player::server::UrlHealthConfig config_;
};

TEST_F(UrlHealthServiceTest, DeletesAfterThreeConsecutiveFailures) {
  std::string error;
  ASSERT_TRUE(repo_.upsert(MakeVideo("bad", "Bad"), &error)) << error;

  FakeProber prober;
  prober.default_ok_ = false;

  video_player::server::UrlHealthService health(&repo_, &prober, config_);
  std::vector<std::int64_t> removed;
  health.set_removed_handler(
      [&](const std::vector<std::int64_t>& ids) { removed = ids; });

  health.runOnce();
  EXPECT_EQ(repo_.listAll(10, 0).size(), 1u);
  EXPECT_EQ(repo_.listAll(10, 0)[0].consecutive_failures, 1);
  EXPECT_TRUE(removed.empty());

  health.runOnce();
  EXPECT_EQ(repo_.listAll(10, 0)[0].consecutive_failures, 2);

  health.runOnce();
  EXPECT_TRUE(repo_.listAll(10, 0).empty());
  ASSERT_EQ(removed.size(), 1u);
}

TEST_F(UrlHealthServiceTest, SuccessResetsFailureCounter) {
  std::string error;
  video_player::server::Video video = MakeVideo("flaky", "Flaky");
  video.consecutive_failures = 2;
  ASSERT_TRUE(repo_.upsert(video, &error)) << error;

  FakeProber prober;
  prober.default_ok_ = true;
  video_player::server::UrlHealthService health(&repo_, &prober, config_);
  health.runOnce();

  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].consecutive_failures, 0);
}

TEST_F(UrlHealthServiceTest, StartStopDoesNotHang) {
  FakeProber prober;
  config_.interval_ms = 50;
  video_player::server::UrlHealthService health(&repo_, &prober, config_);
  health.start();
  health.stop();
  SUCCEED();
}

TEST_F(UrlHealthServiceTest, SkipsLocalUploadEvenIfUnreachable) {
  std::string error;
  video_player::server::Video local = MakeVideo("local-1", "Local");
  local.source_type = "local_upload";
  local.play_url = "http://127.0.0.1:9/media/videos/local-1.mp4";
  local.download_url = local.play_url;
  ASSERT_TRUE(repo_.upsert(local, &error)) << error;

  FakeProber prober;
  prober.default_ok_ = false;
  std::vector<std::int64_t> removed;
  video_player::server::UrlHealthService health(&repo_, &prober, config_);
  health.set_removed_handler(
      [&removed](const std::vector<std::int64_t>& ids) {
        removed.insert(removed.end(), ids.begin(), ids.end());
      });

  for (int i = 0; i < 5; ++i) {
    health.runOnce();
  }

  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].source_type, "local_upload");
  EXPECT_EQ(all[0].consecutive_failures, 0);
  EXPECT_TRUE(removed.empty());
}
