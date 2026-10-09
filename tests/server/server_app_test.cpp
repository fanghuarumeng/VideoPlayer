#include "square_dance/server/ServerApp.h"
#include "square_dance/server/ServerConfig.h"

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
#include <string>

namespace {

std::string WriteFile(const std::string& path, const std::string& contents) {
  std::ofstream out(path.c_str(), std::ios::binary);
  out << contents;
  out.close();
  return path;
}

}  // namespace

TEST(ServerApp, StartsHttpAndIngestsAuthorizedCatalog) {
  const std::string catalog = WriteFile(
      "server_app_test_catalog.json",
      std::string("{\n") +
          "  \"videos\": [{\n"
          "    \"id\": \"app-1\",\n"
          "    \"title\": \"Server App Square Dance\",\n"
          "    \"play_url\": \"https://example.com/app-1.mp4\",\n"
          "    \"download_url\": \"https://example.com/app-1.mp4\"\n"
          "  }]\n"
          "}\n");

  square_dance::server::ServerConfig cfg =
      square_dance::server::MakeDefaultServerConfig();
  cfg.bind_host = "127.0.0.1";
  cfg.http_port = 0;
  cfg.ws_port = 0;
  cfg.sqlite_path = "server_app_test.db";
  cfg.authorized_catalog_path = catalog;
  cfg.ingest_keywords.clear();
  cfg.ingest_keywords.push_back("square");
  cfg.ingest_interval_ms = 60 * 60 * 1000;
  cfg.health_interval_ms = 60 * 60 * 1000;

  std::remove(cfg.sqlite_path.c_str());

  square_dance::server::ServerApp app(cfg);
  std::string error;
  ASSERT_TRUE(app.Start(&error)) << error;
  ASSERT_GT(app.http_port(), 0);
  ASSERT_GT(app.ws_port(), 0);

  httplib::Client client("127.0.0.1", app.http_port());
  client.set_connection_timeout(2, 0);
  client.set_read_timeout(2, 0);
  const httplib::Result res =
      client.Get("/api/v1/videos/search?q=square&limit=10");
  ASSERT_TRUE(res) << "search request failed";
  EXPECT_EQ(res->status, 200);
  const nlohmann::json body = nlohmann::json::parse(res->body);
  ASSERT_TRUE(body.is_array());
  ASSERT_GE(body.size(), 1u);
  EXPECT_EQ(body[0].at("external_id").get<std::string>(), "app-1");

  app.Stop();
  std::remove(cfg.sqlite_path.c_str());
  std::remove(catalog.c_str());
}
