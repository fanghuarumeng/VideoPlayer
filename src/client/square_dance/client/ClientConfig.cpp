#include "square_dance/client/ClientConfig.h"

#include <QFile>

#include <nlohmann/json.hpp>

#include <string>

namespace square_dance {
namespace client {
namespace {

QString FromUtf8(const std::string& s) {
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

bool RequireString(const nlohmann::json& j,
                   const char* key,
                   QString* out,
                   QString* error) {
  if (!j.contains(key) || !j.at(key).is_string()) {
    if (error != NULL) {
      *error = QString::fromUtf8("missing or invalid string field: ") +
               QString::fromUtf8(key);
    }
    return false;
  }
  *out = FromUtf8(j.at(key).get<std::string>());
  return true;
}

}  // namespace

ClientConfig MakeDefaultClientConfig() {
  ClientConfig cfg;
  cfg.http_base_url = QString::fromUtf8("http://127.0.0.1:8080");
  cfg.ws_url = QString::fromUtf8("ws://127.0.0.1:8081");
  cfg.download_dir = QString::fromUtf8("downloads");
  return cfg;
}

bool LoadClientConfigFromString(const QString& json_text,
                                ClientConfig* out,
                                QString* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = QString::fromUtf8("out is null");
    }
    return false;
  }

  const QByteArray utf8 = json_text.toUtf8();
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(utf8.constData(),
                              utf8.constData() + utf8.size());
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = FromUtf8(ex.what());
    }
    return false;
  }

  if (!j.is_object()) {
    if (error != NULL) {
      *error = QString::fromUtf8("config root must be a JSON object");
    }
    return false;
  }

  ClientConfig cfg = MakeDefaultClientConfig();
  if (!RequireString(j, "http_base_url", &cfg.http_base_url, error)) {
    return false;
  }
  if (!RequireString(j, "ws_url", &cfg.ws_url, error)) {
    return false;
  }
  if (!RequireString(j, "download_dir", &cfg.download_dir, error)) {
    return false;
  }

  *out = cfg;
  return true;
}

bool LoadClientConfigFromFile(const QString& path,
                              ClientConfig* out,
                              QString* error) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to open config file: ") + path;
    }
    return false;
  }
  const QByteArray bytes = file.readAll();
  return LoadClientConfigFromString(QString::fromUtf8(bytes), out, error);
}

}  // namespace client
}  // namespace square_dance
