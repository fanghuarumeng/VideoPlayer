#include "square_dance/server/ingest/IngestService.h"
#include "square_dance/server/video_store/SqliteVideoRepository.h"
#include "square_dance/server/video_source/IVideoSource.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

namespace {

class FakeSource : public square_dance::server::IVideoSource {
 public:
  FakeSource() : fail_(false), calls_(0) {}

  virtual std::string id() const { return "fake"; }

  virtual std::vector<square_dance::server::VideoCandidate> search(
      const std::string& /*keyword*/, int /*limit*/, std::string* error) {
    ++calls_;
    if (fail_) {
      if (error != NULL) {
        *error = "fake source failure";
      }
      return std::vector<square_dance::server::VideoCandidate>();
    }
    return candidates_;
  }

  std::vector<square_dance::server::VideoCandidate> candidates_;
  bool fail_;
  int calls_;
};

square_dance::server::VideoCandidate MakeCandidate(const std::string& id,
                                                   const std::string& title) {
  square_dance::server::VideoCandidate c;
  c.external_id = id;
  c.title = title;
  c.play_url = "https://example.com/" + id + ".mp4";
  c.download_url = c.play_url;
  c.source_type = "authorized";
  return c;
}

}  // namespace

class IngestServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = "ingest_service_test.db";
    std::remove(db_path_.c_str());
    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;

    config_ = square_dance::server::IngestConfig::MakeDefault();
    config_.keywords.clear();
    config_.keywords.push_back("dance");
    config_.per_keyword_limit = 10;
    config_.interval_ms = 60 * 60 * 1000;
  }

  void TearDown() override {
    repo_.Close();
    std::remove(db_path_.c_str());
  }

  std::string db_path_;
  square_dance::server::SqliteVideoRepository repo_;
  square_dance::server::IngestConfig config_;
};

TEST_F(IngestServiceTest, RunOnceUpsertsAndDedupes) {
  FakeSource source;
  source.candidates_.push_back(MakeCandidate("a1", "Alpha Dance"));

  std::vector<square_dance::server::IVideoSource*> sources;
  sources.push_back(&source);

  int handler_calls = 0;
  int last_changed = 0;

  square_dance::server::IngestService ingest(sources, &repo_, config_);
  ingest.set_upserted_handler(
      [&](const std::vector<square_dance::server::Video>& videos) {
        ++handler_calls;
        last_changed = static_cast<int>(videos.size());
      });

  ingest.runOnce();
  EXPECT_EQ(repo_.listAll(10, 0).size(), 1u);
  EXPECT_EQ(handler_calls, 1);
  EXPECT_EQ(last_changed, 1);

  ingest.runOnce();
  EXPECT_EQ(repo_.listAll(10, 0).size(), 1u);
  EXPECT_EQ(handler_calls, 1);
}

TEST_F(IngestServiceTest, TitleChangeFiresHandlerAgain) {
  FakeSource source;
  source.candidates_.push_back(MakeCandidate("a1", "Alpha Dance"));

  std::vector<square_dance::server::IVideoSource*> sources;
  sources.push_back(&source);
  square_dance::server::IngestService ingest(sources, &repo_, config_);

  int handler_calls = 0;
  ingest.set_upserted_handler(
      [&](const std::vector<square_dance::server::Video>&) { ++handler_calls; });

  ingest.runOnce();
  source.candidates_[0].title = "Alpha Dance Remix";
  ingest.runOnce();
  EXPECT_EQ(handler_calls, 2);
  EXPECT_EQ(repo_.listAll(10, 0)[0].title, "Alpha Dance Remix");
}

TEST_F(IngestServiceTest, SourceFailureDoesNotAbortOtherSources) {
  FakeSource bad;
  bad.fail_ = true;
  FakeSource good;
  good.candidates_.push_back(MakeCandidate("g1", "Good Dance"));

  std::vector<square_dance::server::IVideoSource*> sources;
  sources.push_back(&bad);
  sources.push_back(&good);

  square_dance::server::IngestService ingest(sources, &repo_, config_);
  ingest.runOnce();
  EXPECT_EQ(repo_.listAll(10, 0).size(), 1u);
}

TEST_F(IngestServiceTest, StartStopDoesNotHang) {
  FakeSource source;
  std::vector<square_dance::server::IVideoSource*> sources;
  sources.push_back(&source);
  config_.interval_ms = 50;
  square_dance::server::IngestService ingest(sources, &repo_, config_);
  ingest.start();
  ingest.stop();
  SUCCEED();
}
