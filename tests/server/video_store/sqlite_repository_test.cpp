#include "video_player/server/video_store/SqliteVideoRepository.h"
#include "video_player/server/video_store/Video.h"

#include "sqlite3.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

namespace {

std::string TempDbPath(const char* name) {
  // ctest runs with cwd = build tree; keep DB next to that.
  return std::string(name);
}

video_player::server::Video MakeVideo(const std::string& source_type,
                                      const std::string& external_id,
                                      const std::string& title) {
  video_player::server::Video v;
  v.id = 0;
  v.external_id = external_id;
  v.title = title;
  v.play_url = "https://example.com/" + external_id + ".mp4";
  v.download_url = v.play_url;
  v.source_type = source_type;
  v.consecutive_failures = 0;
  v.created_at = "2026-10-09T00:00:00Z";
  v.updated_at = "2026-10-09T00:00:00Z";
  v.thumbnail_url = "";
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
  video_player::server::SqliteVideoRepository repo_;
};

TEST_F(SqliteVideoRepositoryTest, UpsertInsertThenUpdateSameKey) {
  std::string error;
  video_player::server::Video a =
      MakeVideo("authorized", "auth-001", "Dance Alpha");
  ASSERT_TRUE(repo_.upsert(a, &error)) << error;

  std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
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

  std::vector<video_player::server::Video> hits =
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
  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);

  video_player::server::Video got;
  ASSERT_TRUE(repo_.getById(all[0].id, &got, &error)) << error;
  EXPECT_EQ(got.title, "To Delete");

  ASSERT_TRUE(repo_.removeById(all[0].id, &error)) << error;
  EXPECT_TRUE(repo_.listAll(10, 0).empty());
  EXPECT_FALSE(repo_.getById(all[0].id, &got, &error));
}

TEST_F(SqliteVideoRepositoryTest, UpsertPersistsThumbnailUrl) {
  std::string error;
  video_player::server::Video a =
      MakeVideo("local_upload", "u-1", "Local Clip");
  a.thumbnail_url = "http://127.0.0.1:8080/media/thumbs/u-1.jpg";
  ASSERT_TRUE(repo_.upsert(a, &error)) << error;

  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0].thumbnail_url, a.thumbnail_url);

  video_player::server::Video got;
  ASSERT_TRUE(repo_.getById(all[0].id, &got, &error)) << error;
  EXPECT_EQ(got.thumbnail_url, a.thumbnail_url);
}

TEST_F(SqliteVideoRepositoryTest, ListAllNewestFirst) {
  std::string error;
  ASSERT_TRUE(repo_.upsert(MakeVideo("authorized", "a", "First"), &error))
      << error;
  ASSERT_TRUE(repo_.upsert(MakeVideo("authorized", "b", "Second"), &error))
      << error;

  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 2u);
  EXPECT_EQ(all[0].external_id, "b");
  EXPECT_EQ(all[1].external_id, "a");
}

TEST_F(SqliteVideoRepositoryTest, MigratesV1SchemaAddsThumbnailUrl) {
  repo_.Close();
  std::remove(db_path_.c_str());

  sqlite3* raw = NULL;
  ASSERT_EQ(sqlite3_open(db_path_.c_str(), &raw), SQLITE_OK);
  const char* v1 =
      "CREATE TABLE schema_version (version INTEGER NOT NULL);"
      "INSERT INTO schema_version(version) VALUES (1);"
      "CREATE TABLE videos ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  external_id TEXT NOT NULL,"
      "  title TEXT NOT NULL,"
      "  play_url TEXT NOT NULL,"
      "  download_url TEXT NOT NULL,"
      "  source_type TEXT NOT NULL,"
      "  consecutive_failures INTEGER NOT NULL DEFAULT 0,"
      "  created_at TEXT NOT NULL,"
      "  updated_at TEXT NOT NULL,"
      "  UNIQUE(source_type, external_id)"
      ");"
      "INSERT INTO videos(external_id,title,play_url,download_url,source_type,"
      " consecutive_failures,created_at,updated_at) VALUES "
      "('legacy','Old','https://e/a.mp4','https://e/a.mp4','authorized',0,"
      "'2026-01-01T00:00:00Z','2026-01-01T00:00:00Z');";
  char* errmsg = NULL;
  ASSERT_EQ(sqlite3_exec(raw, v1, NULL, NULL, &errmsg), SQLITE_OK) << errmsg;
  sqlite3_close(raw);

  std::string error;
  ASSERT_TRUE(repo_.Open(db_path_, &error)) << error;
  const std::vector<video_player::server::Video> all = repo_.listAll(10, 0);
  ASSERT_EQ(all.size(), 1u);
  EXPECT_TRUE(all[0].thumbnail_url.empty());
  EXPECT_EQ(all[0].title, "Old");

  video_player::server::Video updated = all[0];
  updated.thumbnail_url = "http://127.0.0.1:8080/media/thumbs/legacy.jpg";
  ASSERT_TRUE(repo_.upsert(updated, &error)) << error;
  video_player::server::Video got;
  ASSERT_TRUE(repo_.getById(all[0].id, &got, &error)) << error;
  EXPECT_EQ(got.thumbnail_url, updated.thumbnail_url);
}
