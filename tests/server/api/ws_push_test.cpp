#include "square_dance/server/api/WsPushServer.h"
#include "square_dance/server/video_store/Video.h"

#include <gtest/gtest.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

typedef websocketpp::client<websocketpp::config::asio_client> WsClient;

square_dance::server::Video MakeVideo() {
  square_dance::server::Video v;
  v.id = 42;
  v.external_id = "ws-1";
  v.title = "WS Dance";
  v.play_url = "https://example.com/ws.mp4";
  v.download_url = v.play_url;
  v.source_type = "authorized";
  v.consecutive_failures = 0;
  v.created_at = "2026-10-09T00:00:00Z";
  v.updated_at = "2026-10-09T00:00:00Z";
  return v;
}

}  // namespace

TEST(WsPushServer, BroadcastUpsertedReachesClient) {
  square_dance::server::WsPushServer server;
  std::string error;
  ASSERT_TRUE(server.Start("127.0.0.1", 0, &error)) << error;
  ASSERT_GT(server.bound_port(), 0);

  std::mutex mu;
  std::vector<std::string> messages;
  std::atomic<bool> opened(false);

  WsClient client;
  client.clear_access_channels(websocketpp::log::alevel::all);
  client.clear_error_channels(websocketpp::log::elevel::all);
  client.init_asio();
  client.set_open_handler([&](websocketpp::connection_hdl) { opened = true; });
  client.set_message_handler(
      [&](websocketpp::connection_hdl, WsClient::message_ptr msg) {
        std::lock_guard<std::mutex> lock(mu);
        messages.push_back(msg->get_payload());
      });

  std::ostringstream uri;
  uri << "ws://127.0.0.1:" << server.bound_port() << "/ws";

  websocketpp::lib::error_code ec;
  WsClient::connection_ptr con = client.get_connection(uri.str(), ec);
  ASSERT_FALSE(ec) << ec.message();
  client.connect(con);

  std::thread client_thread([&]() { client.run(); });

  for (int i = 0; i < 100 && !opened; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(opened.load()) << "WS client failed to open";

  std::vector<square_dance::server::Video> videos;
  videos.push_back(MakeVideo());
  server.BroadcastUpserted(videos);

  bool got = false;
  for (int i = 0; i < 100; ++i) {
    {
      std::lock_guard<std::mutex> lock(mu);
      if (!messages.empty()) {
        got = true;
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(got) << "no WS message received";

  nlohmann::json msg;
  {
    std::lock_guard<std::mutex> lock(mu);
    msg = nlohmann::json::parse(messages[0]);
  }
  EXPECT_EQ(msg.at("type").get<std::string>(), "video.upserted");
  EXPECT_EQ(msg.at("video").at("external_id").get<std::string>(), "ws-1");

  client.stop();
  if (client_thread.joinable()) {
    client_thread.join();
  }
  server.Stop();
}
