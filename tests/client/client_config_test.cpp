#include "video_player/client/ClientConfig.h"
#include "video_player/client/MainWindow.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <string>

namespace {

std::string FixturePath(const char* name) {
#ifndef VIDEO_PLAYER_SOURCE_DIR
#error VIDEO_PLAYER_SOURCE_DIR must be defined by CMake
#endif
  return std::string(VIDEO_PLAYER_SOURCE_DIR) + "/tests/client/fixtures/" + name;
}

}  // namespace

TEST(ClientConfig, LoadsValidJsonFixture) {
  video_player::client::ClientConfig cfg;
  QString error;
  const QString path = QString::fromUtf8(FixturePath("client_config_valid.json").c_str());
  ASSERT_TRUE(video_player::client::LoadClientConfigFromFile(path, &cfg, &error))
      << error.toStdString();

  EXPECT_EQ(cfg.http_base_url, QString::fromUtf8("http://127.0.0.1:8080"));
  EXPECT_EQ(cfg.ws_url, QString::fromUtf8("ws://127.0.0.1:8081/ws"));
  EXPECT_EQ(cfg.download_dir, QString::fromUtf8("downloads"));
}

TEST(ClientConfig, FailsWhenHttpBaseUrlMissing) {
  const QString json = QString::fromUtf8(
      "{\"ws_url\":\"ws://127.0.0.1:8081\",\"download_dir\":\"downloads\"}");
  video_player::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      video_player::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsWhenWsUrlMissing) {
  const QString json = QString::fromUtf8(
      "{\"http_base_url\":\"http://127.0.0.1:8080\",\"download_dir\":\"d\"}");
  video_player::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      video_player::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsWhenDownloadDirMissing) {
  const QString json = QString::fromUtf8(
      "{\"http_base_url\":\"http://127.0.0.1:8080\",\"ws_url\":\"ws://x\"}");
  video_player::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      video_player::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsOnInvalidJson) {
  video_player::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(video_player::client::LoadClientConfigFromString(
      QString::fromUtf8("{not-json"), &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(MainWindow, ConstructSmoke) {
  video_player::client::MainWindow window;
  window.show();
  EXPECT_TRUE(window.isVisible());
  EXPECT_FALSE(window.windowTitle().isEmpty());
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
