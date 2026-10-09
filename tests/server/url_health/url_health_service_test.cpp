#include "square_dance/server/url_health/UrlHealthService.h"
#include "square_dance/server/video_store/SqliteVideoRepository.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <map>
#include <string>

namespace {

class FakeProber : public square_dance::server::IUrlProber {
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

class UrlHealthServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = "url_health_service_test.db";
    std::remove(db_path_.c_str());
    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
    config_ = square_dance::server::UrlHealthConfig::MakeDefault();
    config_.interval_ms = 60 * 60 * 1000;
  }

  void TearDown() override {
    repo_.Close();
    std::remove(db_path_.c_str());
  }

  std::string db_path_;
  square_dance::server::SqliteVideoRepository repo_;
  square_dance::server::UrlHealthConfig config_;
};

TEST_F(UrlHealthServiceTest, DeletesAfterThreeConsecutiveFailures) {
  std::string error;
  ASSERT_TRUE(repo_.upsert(MakeVideo("bad", "Bad"), &error)) << error;

  FakeProber prober;
  prober.default_ok_ = false;

  square_dance::server::UrlHealthService health(&repo_, &prober, config_);
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
  square_dance::server::Video video = MakeVideo("flaky", "Flaky");
  video.consecutive_failures = 2;
  ASSERT_TRUE(repo_.upsert(video, &error)) << error;

  FakeProber prober;
  prober.default_ok_ = true;
  square_dance::server::UrlHealthService health(&repo_, &prober, config_);
  health.runOnce();

  const std::vector<square_dance::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].consecutive_failures, 0);
}

TEST_F(UrlHealthServiceTest, StartStopDoesNotHang) {
  FakeProber prober;
  config_.interval_ms = 50;
  square_dance::server::UrlHealthService health(&repo_, &prober, config_);
  health.start();
  health.stop();
  SUCCEED();
}
