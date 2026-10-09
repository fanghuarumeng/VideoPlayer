#include "square_dance/server/video_store/SqliteVideoRepository.h"
#include "square_dance/server/video_store/Video.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

namespace {

std::string TempDbPath(const char* name) {
  // ctest runs with cwd = build tree; keep DB next to that.
  return std::string(name);
}

square_dance::server::Video MakeVideo(const std::string& source_type,
                                      const std::string& external_id,
                                      const std::string& title) {
  square_dance::server::Video v;
  v.id = 0;
  v.external_id = external_id;
  v.title = title;
  v.play_url = "https://example.com/" + external_id + ".mp4";
  v.download_url = v.play_url;
  v.source_type = source_type;
  v.consecutive_failures = 0;
  v.created_at = "2026-10-09T00:00:00Z";
  v.updated_at = "2026-10-09T00:00:00Z";
  return v;
}

}  // namespace

class SqliteVideoRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    db_path_ = TempDbPath("video_store_test.db");
    std::remove(db_path_.c_str());
    std::string error;
    ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
  }

  void TearDown() override {
    repo_.Close();
    std::remove(db_path_.c_str());
  }

  std::string db_path_;
  square_dance::server::SqliteVideoRepository repo_;
};

TEST_F(SqliteVideoRepositoryTest, UpsertInsertThenUpdateSameKey) {
  std::string error;
  square_dance::server::Video a =
      MakeVideo("authorized", "auth-001", "Dance Alpha");
  ASSERT_TRUE(repo_.upsert(a, &error)) << error;

  std::vector<square_dance::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].title, "Dance Alpha");
  const std::int64_t id = all[0].id;
  EXPECT_GT(id, 0);

  a.title = "Dance Alpha Updated";
  a.updated_at = "2026-10-09T12:00:00Z";
  ASSERT_TRUE(repo_.upsert(a, &error)) << error;

  all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].id, id);
  EXPECT_EQ(all[0].title, "Dance Alpha Updated");
  EXPECT_EQ(all[0].updated_at, "2026-10-09T12:00:00Z");
}

TEST_F(SqliteVideoRepositoryTest, SearchByNameSubstringCaseInsensitive) {
  std::string error;
  ASSERT_TRUE(
      repo_.upsert(MakeVideo("authorized", "1", "Morning Square Dance"), &error))
      << error;
  ASSERT_TRUE(
      repo_.upsert(MakeVideo("archive_org", "2", "Other Title"), &error))
      << error;

  std::vector<square_dance::server::Video> hits =
      repo_.searchByName("square", 10);
  ASSERT_EQ(hits.size(), 1u);
  EXPECT_EQ(hits[0].external_id, "1");

  hits = repo_.searchByName("SQUARE", 10);
  ASSERT_EQ(hits.size(), 1u);

  EXPECT_TRUE(repo_.searchByName("", 10).empty());
}

TEST_F(SqliteVideoRepositoryTest, GetByIdAndRemoveById) {
  std::string error;
  ASSERT_TRUE(
      repo_.upsert(MakeVideo("authorized", "x", "To Delete"), &error))
      << error;
  const std::vector<square_dance::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);

  square_dance::server::Video got;
  ASSERT_TRUE(repo_.getById(all[0].id, &got, &error)) << error;
  EXPECT_EQ(got.title, "To Delete");

  ASSERT_TRUE(repo_.removeById(all[0].id, &error)) << error;
  EXPECT_TRUE(repo_.listAll(10, 0).empty());
  EXPECT_FALSE(repo_.getById(all[0].id, &got, &error));
}
