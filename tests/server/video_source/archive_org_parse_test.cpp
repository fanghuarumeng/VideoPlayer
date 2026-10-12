#include "video_player/server/video_source/ArchiveOrgSource.h"

#include <gtest/gtest.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace {

std::string ReadFixture(const char* name) {
#ifndef VIDEO_PLAYER_SOURCE_DIR
#error VIDEO_PLAYER_SOURCE_DIR must be defined by CMake
#endif
  const std::string path = std::string(VIDEO_PLAYER_SOURCE_DIR) +
                           "/tests/server/video_source/fixtures/" + name;
  std::ifstream in(path.c_str(), std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

TEST(ArchiveOrgParse, ParsesSearchDocs) {
  std::vector<video_player::server::ArchiveOrgSearchHit> hits;
  std::string error;
  ASSERT_TRUE(video_player::server::ArchiveOrgSource::ParseSearchResponse(
      ReadFixture("archive_org_search.json"), &hits, &error))
      << error;
  ASSERT_EQ(hits.size(), 2u);
  EXPECT_EQ(hits[0].identifier, "WartimeN1943");
  EXPECT_EQ(hits[0].title, "Wartime Nutrition");
}

TEST(ArchiveOrgParse, PrefersMp4AndBuildsDownloadUrl) {
  std::string error;
  const std::string file =
      video_player::server::ArchiveOrgSource::PickMediaFileName(
          ReadFixture("archive_org_metadata_mp4.json"), &error);
  EXPECT_EQ(file, "WartimeN1943.mp4") << error;
  EXPECT_EQ(video_player::server::ArchiveOrgSource::BuildDownloadUrl(
                "WartimeN1943", file),
            "https://archive.org/download/WartimeN1943/WartimeN1943.mp4");
}

TEST(ArchiveOrgParse, ReturnsEmptyWhenNoVideoFiles) {
  std::string error;
  const std::string file =
      video_player::server::ArchiveOrgSource::PickMediaFileName(
          ReadFixture("archive_org_metadata_novideo.json"), &error);
  EXPECT_TRUE(file.empty());
  EXPECT_FALSE(error.empty());
}

TEST(ArchiveOrgParse, SearchWithInjectedHttpResolvesCandidate) {
  const std::string search_json = ReadFixture("archive_org_search.json");
  const std::string meta_json = ReadFixture("archive_org_metadata_mp4.json");
  const std::string empty_meta = ReadFixture("archive_org_metadata_novideo.json");

  video_player::server::ArchiveOrgSource::HttpGetFn getter =
      [&](const std::string& url, std::string* body, std::string* error) -> bool {
        if (url.find("advancedsearch.php") != std::string::npos) {
          *body = search_json;
          return true;
        }
        if (url.find("/metadata/WartimeN1943") != std::string::npos) {
          *body = meta_json;
          return true;
        }
        if (url.find("/metadata/NoVideoItem") != std::string::npos) {
          *body = empty_meta;
          return true;
        }
        if (error != NULL) {
          *error = "unexpected url: " + url;
        }
        return false;
      };

  video_player::server::ArchiveOrgSource source(getter);
  EXPECT_EQ(source.id(), "archive_org");

  std::string error;
  const std::vector<video_player::server::VideoCandidate> candidates =
      source.search("square dance", 10, &error);
  ASSERT_EQ(candidates.size(), 1u) << error;
  EXPECT_EQ(candidates[0].external_id, "WartimeN1943");
  EXPECT_EQ(candidates[0].source_type, "archive_org");
  EXPECT_EQ(candidates[0].play_url,
            "https://archive.org/download/WartimeN1943/WartimeN1943.mp4");
  EXPECT_EQ(candidates[0].download_url, candidates[0].play_url);
}
