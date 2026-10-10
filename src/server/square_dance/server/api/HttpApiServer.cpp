#include "square_dance/server/api/HttpApiServer.h"

#include "square_dance/server/api/video_json.h"
#include "square_dance/server/log.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

namespace square_dance {
namespace server {

struct HttpApiServer::Impl {
  httplib::Server server;
};

HttpApiServer::HttpApiServer(IVideoRepository* repo, IngestService* ingest)
    : repo_(repo),
      ingest_(ingest),
      requested_port_(0),
      bound_port_(0),
      stop_(true),
      impl_(new Impl()) {}

HttpApiServer::~HttpApiServer() {
  Stop();
}

int HttpApiServer::bound_port() const {
  return bound_port_.load();
}

void HttpApiServer::RegisterRoutes() {
  httplib::Server& svr = impl_->server;

  svr.Get("/api/v1/videos/search",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (repo_ == NULL) {
              res.status = 500;
              res.set_content("{\"error\":\"repository unavailable\"}",
                              "application/json");
              return;
            }
            const std::string q = req.has_param("q") ? req.get_param_value("q") : "";
            int limit = 20;
            if (req.has_param("limit")) {
              limit = std::atoi(req.get_param_value("limit").c_str());
              if (limit <= 0) {
                limit = 20;
              }
            }
            const std::vector<Video> videos = repo_->searchByName(q, limit);
            res.status = 200;
            res.set_content(VideoListToApiJson(videos).dump(),
                            "application/json; charset=utf-8");
          });

  svr.Get("/api/v1/videos",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (repo_ == NULL) {
              res.status = 500;
              res.set_content("{\"error\":\"repository unavailable\"}",
                              "application/json");
              return;
            }
            int limit = 50;
            int offset = 0;
            if (req.has_param("limit")) {
              limit = std::atoi(req.get_param_value("limit").c_str());
            }
            if (req.has_param("offset")) {
              offset = std::atoi(req.get_param_value("offset").c_str());
            }
            if (limit <= 0) {
              limit = 50;
            }
            if (limit > 200) {
              limit = 200;
            }
            if (offset < 0) {
              offset = 0;
            }
            const std::vector<Video> videos = repo_->listAll(limit, offset);
            res.status = 200;
            res.set_content(VideoListToApiJson(videos).dump(),
                            "application/json; charset=utf-8");
          });

  svr.Get(R"(/api/v1/videos/(\d+))",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (repo_ == NULL) {
              res.status = 500;
              return;
            }
            const std::int64_t id =
                static_cast<std::int64_t>(std::atoll(req.matches[1].str().c_str()));
            Video video;
            std::string error;
            if (!repo_->getById(id, &video, &error)) {
              res.status = 404;
              res.set_content("{\"error\":\"not found\"}", "application/json");
              return;
            }
            res.status = 200;
            res.set_content(VideoToApiJson(video).dump(),
                            "application/json; charset=utf-8");
          });

  svr.Post("/api/v1/ingest/run",
           [this](const httplib::Request&, httplib::Response& res) {
             if (ingest_ == NULL) {
               res.status = 503;
               res.set_content("{\"error\":\"ingest unavailable\"}",
                               "application/json");
               return;
             }
             ingest_->runOnce();
             res.status = 200;
             res.set_content("{\"ok\":true}", "application/json");
           });
}

bool HttpApiServer::Start(const std::string& host, int port, std::string* error) {
  if (thread_.joinable()) {
    if (error != NULL) {
      *error = "server already started";
    }
    return false;
  }

  host_ = host;
  requested_port_ = port;
  stop_ = false;
  impl_.reset(new Impl());
  RegisterRoutes();

  int bound = 0;
  if (port == 0) {
    bound = impl_->server.bind_to_any_port(host.c_str());
  } else if (!impl_->server.bind_to_port(host.c_str(), port)) {
    if (error != NULL) {
      *error = "failed to bind HTTP port";
    }
    return false;
  } else {
    bound = port;
  }

  if (bound < 0) {
    if (error != NULL) {
      *error = "failed to bind HTTP port";
    }
    return false;
  }

  bound_port_ = bound;
  thread_ = std::thread(&HttpApiServer::ServeLoop, this);

  // Brief wait so listen is up for tests.
  for (int i = 0; i < 50; ++i) {
    if (impl_->server.is_running()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  LogInfo(std::string("HTTP API listening on ") + host_ + ":" +
          std::to_string(static_cast<long long>(bound_port_.load())));
  return true;
}

void HttpApiServer::ServeLoop() {
  impl_->server.listen_after_bind();
}

void HttpApiServer::Stop() {
  stop_ = true;
  if (impl_.get() != NULL) {
    impl_->server.stop();
  }
  if (thread_.joinable()) {
    thread_.join();
  }
  bound_port_ = 0;
}

}  // namespace server
}  // namespace square_dance
