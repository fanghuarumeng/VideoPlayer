#include "square_dance/client/ClientConfig.h"
#include "square_dance/client/MainWindow.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <string>

namespace {

std::string FixturePath(const char* name) {
#ifndef SQUARE_DANCE_SOURCE_DIR
#error SQUARE_DANCE_SOURCE_DIR must be defined by CMake
#endif
  return std::string(SQUARE_DANCE_SOURCE_DIR) + "/tests/client/fixtures/" + name;
}

}  // namespace

TEST(ClientConfig, LoadsValidJsonFixture) {
  square_dance::client::ClientConfig cfg;
  QString error;
  const QString path = QString::fromUtf8(FixturePath("client_config_valid.json").c_str());
  ASSERT_TRUE(square_dance::client::LoadClientConfigFromFile(path, &cfg, &error))
      << error.toStdString();

  EXPECT_EQ(cfg.http_base_url, QString::fromUtf8("http://127.0.0.1:8080"));
  EXPECT_EQ(cfg.ws_url, QString::fromUtf8("ws://127.0.0.1:8081/ws"));
  EXPECT_EQ(cfg.download_dir, QString::fromUtf8("downloads"));
}

TEST(ClientConfig, FailsWhenHttpBaseUrlMissing) {
  const QString json = QString::fromUtf8(
      "{\"ws_url\":\"ws://127.0.0.1:8081\",\"download_dir\":\"downloads\"}");
  square_dance::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      square_dance::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsWhenWsUrlMissing) {
  const QString json = QString::fromUtf8(
      "{\"http_base_url\":\"http://127.0.0.1:8080\",\"download_dir\":\"d\"}");
  square_dance::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      square_dance::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsWhenDownloadDirMissing) {
  const QString json = QString::fromUtf8(
      "{\"http_base_url\":\"http://127.0.0.1:8080\",\"ws_url\":\"ws://x\"}");
  square_dance::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(
      square_dance::client::LoadClientConfigFromString(json, &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(ClientConfig, FailsOnInvalidJson) {
  square_dance::client::ClientConfig cfg;
  QString error;
  EXPECT_FALSE(square_dance::client::LoadClientConfigFromString(
      QString::fromUtf8("{not-json"), &cfg, &error));
  EXPECT_FALSE(error.isEmpty());
}

TEST(MainWindow, ConstructSmoke) {
  square_dance::client::MainWindow window;
  window.show();
  EXPECT_TRUE(window.isVisible());
  EXPECT_FALSE(window.windowTitle().isEmpty());
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
