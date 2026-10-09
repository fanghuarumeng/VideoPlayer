#include "square_dance/server/api/WsPushServer.h"

#include "square_dance/server/api/video_json.h"
#include "square_dance/server/log.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <nlohmann/json.hpp>

#include <mutex>
#include <set>
#include <sstream>

namespace square_dance {
namespace server {
namespace {

typedef websocketpp::server<websocketpp::config::asio> WsServer;
typedef websocketpp::connection_hdl ConnectionHdl;

struct ConnectionHdlCompare {
  bool operator()(const ConnectionHdl& a, const ConnectionHdl& b) const {
    return a.lock() < b.lock();
  }
};

}  // namespace

struct WsPushServer::Impl {
  WsServer server;
  std::mutex connections_mutex;
  std::set<ConnectionHdl, ConnectionHdlCompare> connections;
};

WsPushServer::WsPushServer()
    : impl_(new Impl()), bound_port_(0), running_(false) {}

WsPushServer::~WsPushServer() {
  Stop();
}

int WsPushServer::bound_port() const {
  return bound_port_.load();
}

bool WsPushServer::Start(const std::string& host, int port, std::string* error) {
  if (running_) {
    if (error != NULL) {
      *error = "ws server already started";
    }
    return false;
  }

  impl_.reset(new Impl());
  WsServer& server = impl_->server;

  server.clear_access_channels(websocketpp::log::alevel::all);
  server.clear_error_channels(websocketpp::log::elevel::all);
  server.init_asio();
  server.set_reuse_addr(true);

  server.set_validate_handler(
      [this](ConnectionHdl hdl) {
        WsServer::connection_ptr con = impl_->server.get_con_from_hdl(hdl);
        return con->get_resource() == "/ws";
      });

  server.set_open_handler([this](ConnectionHdl hdl) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    impl_->connections.insert(hdl);
  });

  server.set_close_handler([this](ConnectionHdl hdl) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    impl_->connections.erase(hdl);
  });

  server.set_message_handler(
      [](ConnectionHdl, WsServer::message_ptr) {
        // Ignore unknown client messages (ping/app traffic) in v1.
      });

  websocketpp::lib::error_code ec;
  websocketpp::lib::asio::ip::address address;
  try {
    address = websocketpp::lib::asio::ip::address::from_string(host);
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = std::string("invalid WS host: ") + ex.what();
    }
    return false;
  }

  const unsigned short listen_port =
      static_cast<unsigned short>(port <= 0 ? 0 : port);
  const websocketpp::lib::asio::ip::tcp::endpoint endpoint(address, listen_port);
  server.listen(endpoint, ec);
  if (ec) {
    if (error != NULL) {
      *error = "WS listen failed: " + ec.message();
    }
    return false;
  }

  websocketpp::lib::error_code local_ec;
  const websocketpp::lib::asio::ip::tcp::endpoint local =
      server.get_local_endpoint(local_ec);
  if (local_ec) {
    if (error != NULL) {
      *error = "WS get_local_endpoint failed: " + local_ec.message();
    }
    return false;
  }
  bound_port_ = static_cast<int>(local.port());

  server.start_accept(ec);
  if (ec) {
    if (error != NULL) {
      *error = "WS start_accept failed: " + ec.message();
    }
    return false;
  }

  running_ = true;
  thread_ = std::thread([this]() {
    try {
      impl_->server.run();
    } catch (const std::exception& ex) {
      LogError(std::string("WS server run exception: ") + ex.what());
    }
  });

  std::ostringstream ss;
  ss << "WS push listening on " << host << ":" << bound_port_.load() << "/ws";
  LogInfo(ss.str());
  return true;
}

void WsPushServer::Stop() {
  if (!running_) {
    return;
  }
  running_ = false;
  websocketpp::lib::error_code ec;
  impl_->server.stop_listening(ec);
  {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (std::set<ConnectionHdl, ConnectionHdlCompare>::iterator it =
             impl_->connections.begin();
         it != impl_->connections.end(); ++it) {
      websocketpp::lib::error_code close_ec;
      impl_->server.close(*it, websocketpp::close::status::going_away, "",
                          close_ec);
    }
    impl_->connections.clear();
  }
  impl_->server.stop();
  if (thread_.joinable()) {
    thread_.join();
  }
  bound_port_ = 0;
}

void WsPushServer::BroadcastUpserted(const std::vector<Video>& videos) {
  if (!running_ || videos.empty()) {
    return;
  }
  std::vector<std::string> payloads;
  payloads.reserve(videos.size());
  for (std::size_t i = 0; i < videos.size(); ++i) {
    nlohmann::json msg;
    msg["type"] = "video.upserted";
    msg["video"] = VideoToApiJson(videos[i]);
    payloads.push_back(msg.dump());
  }

  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  for (std::set<ConnectionHdl, ConnectionHdlCompare>::iterator it =
           impl_->connections.begin();
       it != impl_->connections.end(); ++it) {
    for (std::size_t i = 0; i < payloads.size(); ++i) {
      websocketpp::lib::error_code ec;
      impl_->server.send(*it, payloads[i], websocketpp::frame::opcode::text, ec);
    }
  }
}

void WsPushServer::BroadcastRemoved(const std::vector<std::int64_t>& ids) {
  if (!running_ || ids.empty()) {
    return;
  }
  std::vector<std::string> payloads;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    nlohmann::json msg;
    msg["type"] = "video.removed";
    msg["id"] = ids[i];
    payloads.push_back(msg.dump());
  }

  std::lock_guard<std::mutex> lock(impl_->connections_mutex);
  for (std::set<ConnectionHdl, ConnectionHdlCompare>::iterator it =
           impl_->connections.begin();
       it != impl_->connections.end(); ++it) {
    for (std::size_t i = 0; i < payloads.size(); ++i) {
      websocketpp::lib::error_code ec;
      impl_->server.send(*it, payloads[i], websocketpp::frame::opcode::text, ec);
    }
  }
}

}  // namespace server
}  // namespace square_dance
