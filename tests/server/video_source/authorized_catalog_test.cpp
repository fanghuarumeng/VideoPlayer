#include "square_dance/server/video_source/AuthorizedCatalogSource.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace {

std::string WriteTempCatalog(const std::string& json) {
  const std::string path = "authorized_catalog_test.json";
  std::ofstream out(path.c_str(), std::ios::binary);
  out << json;
  out.close();
  return path;
}

}  // namespace

TEST(AuthorizedCatalog, ParseSkipsEntriesWithoutHttpUrl) {
  const std::string json = R"({
    "videos": [
      {
        "id": "good",
        "title": "Good Dance",
        "play_url": "https://example.com/a.mp4",
        "download_url": "https://example.com/a.mp4"
      },
      {
        "id": "bad",
        "title": "Missing Url",
        "play_url": "",
        "download_url": ""
      },
      {
        "id": "ftp",
        "title": "Ftp Dance",
        "play_url": "ftp://example.com/a.mp4",
        "download_url": "ftp://example.com/a.mp4"
      }
    ]
  })";

  std::vector<square_dance::server::VideoCandidate> out;
  std::string error;
  ASSERT_TRUE(square_dance::server::AuthorizedCatalogSource::ParseCatalogJson(
      json, &out, &error))
      << error;
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0].external_id, "good");
  EXPECT_EQ(out[0].source_type, "authorized");
}

TEST(AuthorizedCatalog, SearchFiltersByTitleKeyword) {
  const std::string json = R"({
    "videos": [
      {
        "id": "1",
        "title": "Morning Square Dance",
        "play_url": "https://example.com/1.mp4",
        "download_url": "https://example.com/1.mp4"
      },
      {
        "id": "2",
        "title": "Yoga Flow",
        "play_url": "https://example.com/2.mp4",
        "download_url": "https://example.com/2.mp4"
      }
    ]
  })";
  const std::string path = WriteTempCatalog(json);
  square_dance::server::AuthorizedCatalogSource source(path);
  EXPECT_EQ(source.id(), "authorized");

  std::string error;
  std::vector<square_dance::server::VideoCandidate> hits =
      source.search("square", 10, &error);
  ASSERT_EQ(hits.size(), 1u) << error;
  EXPECT_EQ(hits[0].external_id, "1");

  hits = source.search("yoga", 10, &error);
  ASSERT_EQ(hits.size(), 1u);
  EXPECT_EQ(hits[0].external_id, "2");

  std::remove(path.c_str());
}

TEST(AuthorizedCatalog, RejectsMalformedRoot) {
  std::vector<square_dance::server::VideoCandidate> out;
  std::string error;
  EXPECT_FALSE(square_dance::server::AuthorizedCatalogSource::ParseCatalogJson(
      "[]", &out, &error));
  EXPECT_FALSE(error.empty());
}
