#include "square_dance/server/api/HttpApiServer.h"

#include "square_dance/server/api/video_json.h"
#include "square_dance/server/log.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "httplib.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

namespace square_dance {
namespace server {
namespace {

const std::size_t kMaxUploadBytes = static_cast<std::size_t>(2ull * 1024 * 1024 * 1024);

struct GateHolder {
  UploadGate* gate;
  explicit GateHolder(UploadGate* g) : gate(g) {}
  ~GateHolder() {
    if (gate != NULL) {
      gate->release();
    }
  }
};

// Stream file bytes (supports HTTP Range via httplib content_provider).
// Avoids loading a 100MB+ video into RAM before the first frame can play.
bool ServeFileRanged(const std::string& path, const std::string& content_type,
                     httplib::Response& res) {
  std::ifstream probe(path.c_str(),
                      std::ios::in | std::ios::binary | std::ios::ate);
  if (!probe) {
    return false;
  }
  const std::streamoff end_pos = probe.tellg();
  if (end_pos < 0) {
    return false;
  }
  const std::size_t file_size = static_cast<std::size_t>(end_pos);
  probe.close();

  std::shared_ptr<std::ifstream> file(new std::ifstream(
      path.c_str(), std::ios::in | std::ios::binary));
  if (!file->good()) {
    return false;
  }

  // Leave status at -1 so httplib can promote Range requests to 206.
  res.set_header("Accept-Ranges", "bytes");
  res.set_header("Cache-Control", "private, max-age=0");
  res.set_content_provider(
      file_size, content_type,
      [file](size_t offset, size_t length, httplib::DataSink& sink) {
        if (!file) {
          return false;
        }
        file->clear();
        file->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!(*file)) {
          return false;
        }
        std::vector<char> buf(64 * 1024);
        size_t remaining = length;
        while (remaining > 0) {
          const size_t chunk =
              remaining < buf.size() ? remaining : buf.size();
          file->read(&buf[0], static_cast<std::streamsize>(chunk));
          const std::streamsize got = file->gcount();
          if (got <= 0) {
            break;
          }
          if (!sink.write(&buf[0], static_cast<size_t>(got))) {
            return false;
          }
          remaining -= static_cast<size_t>(got);
          if (static_cast<size_t>(got) < chunk) {
            break;
          }
        }
        return true;
      });
  return true;
}

bool IsSafeMediaName(const std::string& name) {
  if (name.empty() || name.size() > 256) {
    return false;
  }
  for (std::size_t i = 0; i < name.size(); ++i) {
    const char c = name[i];
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
    if (!ok) {
      return false;
    }
  }
  if (name.find("..") != std::string::npos) {
    return false;
  }
  return true;
}

}  // namespace

struct HttpApiServer::Impl {
  httplib::Server server;
};

HttpApiServer::HttpApiServer(IVideoRepository* repo, IngestService* ingest)
    : repo_(repo),
      ingest_(ingest),
      media_store_(NULL),
      upload_gate_(NULL),
      thumb_queue_(NULL),
      requested_port_(0),
      bound_port_(0),
      stop_(true),
      impl_(new Impl()) {}

HttpApiServer::~HttpApiServer() {
  Stop();
}

void HttpApiServer::set_local_media(LocalMediaStore* store, UploadGate* gate) {
  media_store_ = store;
  upload_gate_ = gate;
}

void HttpApiServer::set_thumbnail_queue(ThumbnailQueue* queue) {
  thumb_queue_ = queue;
}

void HttpApiServer::set_upserted_handler(UpsertedHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  upserted_handler_ = handler;
}

void HttpApiServer::NotifyUpserted(const Video& video) {
  UpsertedHandler handler;
  {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    handler = upserted_handler_;
  }
  if (handler) {
    handler(video);
  }
}

void HttpApiServer::RefreshHttpBase() {
  if (media_store_ == NULL) {
    return;
  }
  std::ostringstream ss;
  ss << "http://" << host_ << ":" << bound_port_.load();
  media_store_->set_http_base(ss.str());
}

int HttpApiServer::bound_port() const {
  return bound_port_.load();
}

void HttpApiServer::RegisterRoutes() {
  httplib::Server& svr = impl_->server;
  svr.set_payload_max_length(kMaxUploadBytes);

  svr.Get("/api/v1/videos/search",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (repo_ == NULL) {
              res.status = 500;
              res.set_content("{\"error\":\"repository unavailable\"}",
                              "application/json");
              return;
            }
            const std::string q =
                req.has_param("q") ? req.get_param_value("q") : "";
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
            const std::int64_t id = static_cast<std::int64_t>(
                std::atoll(req.matches[1].str().c_str()));
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

  svr.Post("/api/v1/videos/upload",
           [this](const httplib::Request& req, httplib::Response& res) {
             if (media_store_ == NULL || upload_gate_ == NULL) {
               res.status = 503;
               res.set_content("{\"error\":\"upload unavailable\"}",
                               "application/json");
               return;
             }
             if (!upload_gate_->tryAcquire()) {
               res.status = 503;
               res.set_content("{\"error\":\"upload_busy\"}",
                               "application/json");
               return;
             }
             GateHolder holder(upload_gate_);

             if (!req.is_multipart_form_data() || !req.has_file("file")) {
               res.status = 400;
               res.set_content("{\"error\":\"missing file field\"}",
                               "application/json");
               return;
             }

             const httplib::MultipartFormData& file = req.get_file_value("file");
             if (file.content.size() > kMaxUploadBytes) {
               res.status = 413;
               res.set_content("{\"error\":\"file too large\"}",
                               "application/json");
               return;
             }

             std::string title;
             if (req.has_file("title")) {
               title = req.get_file_value("title").content;
             } else if (req.has_param("title")) {
               title = req.get_param_value("title");
             }

             RefreshHttpBase();
             Video video;
             std::string error;
             if (!media_store_->storeUpload(file.filename, title, file.content,
                                           &video, &error)) {
               const bool bad_ext =
                   error.find("extension") != std::string::npos ||
                   error.find("empty file") != std::string::npos;
               res.status = bad_ext ? 400 : 500;
               nlohmann::json body;
               body["error"] = error;
               res.set_content(body.dump(), "application/json");
               return;
             }

             NotifyUpserted(video);

             if (thumb_queue_ != NULL) {
               const std::string ext =
                   LocalMediaStore::ExtensionOf(file.filename);
               const std::string video_name = video.external_id + ext;
               const std::string thumb_name = video.external_id + ".jpg";
               ThumbJob job;
               job.video_id = video.id;
               job.video_path =
                   media_store_->videos_dir() + "/" + video_name;
               job.thumb_path =
                   media_store_->thumbs_dir() + "/" + thumb_name;
               job.thumb_filename = thumb_name;
               thumb_queue_->enqueue(job);
             }

             res.status = 201;
             res.set_content(VideoToApiJson(video).dump(),
                             "application/json; charset=utf-8");
           });

  svr.Get(R"(/media/videos/(.+))",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (media_store_ == NULL) {
              res.status = 404;
              return;
            }
            const std::string name = req.matches[1].str();
            if (!IsSafeMediaName(name)) {
              res.status = 400;
              res.set_content("{\"error\":\"invalid name\"}",
                              "application/json");
              return;
            }
            const std::string path = media_store_->videos_dir() + "/" + name;
            const std::string ext = LocalMediaStore::ExtensionOf(name);
            const std::string ctype =
                LocalMediaStore::ContentTypeForExtension(ext);
            if (!ServeFileRanged(path, ctype, res)) {
              res.status = 404;
              res.set_content("{\"error\":\"not found\"}", "application/json");
            }
          });

  svr.Get(R"(/media/thumbs/(.+))",
          [this](const httplib::Request& req, httplib::Response& res) {
            if (media_store_ == NULL) {
              res.status = 404;
              return;
            }
            const std::string name = req.matches[1].str();
            if (!IsSafeMediaName(name)) {
              res.status = 400;
              res.set_content("{\"error\":\"invalid name\"}",
                              "application/json");
              return;
            }
            const std::string path = media_store_->thumbs_dir() + "/" + name;
            std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
            if (!in) {
              res.status = 404;
              res.set_content("{\"error\":\"not found\"}", "application/json");
              return;
            }
            std::ostringstream ss;
            ss << in.rdbuf();
            res.status = 200;
            res.set_content(ss.str(), "image/jpeg");
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
  RefreshHttpBase();
  thread_ = std::thread(&HttpApiServer::ServeLoop, this);

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
