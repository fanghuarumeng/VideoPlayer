#include "square_dance/client/api/push_json.h"

#include "square_dance/client/api/catalog_json.h"

#include <nlohmann/json.hpp>

#include <string>

namespace square_dance {
namespace client {
namespace {

QString FromUtf8(const std::string& s) {
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

}  // namespace

bool ParsePushMessage(const QByteArray& json, PushMessage* out, QString* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = QString::fromUtf8("out is null");
    }
    return false;
  }

  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json.constData(), json.constData() + json.size());
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = FromUtf8(ex.what());
    }
    return false;
  }

  if (!j.is_object() || !j.contains("type") || !j.at("type").is_string()) {
    if (error != NULL) {
      *error = QString::fromUtf8("push message missing string type");
    }
    return false;
  }

  const std::string type = j.at("type").get<std::string>();
  PushMessage msg;

  if (type == "video.upserted") {
    if (!j.contains("video")) {
      if (error != NULL) {
        *error = QString::fromUtf8("video.upserted missing video");
      }
      return false;
    }
    const std::string video_text = j.at("video").dump();
    Video video;
    QString parse_error;
    if (!ParseVideoJson(QByteArray(video_text.data(),
                                   static_cast<int>(video_text.size())),
                        &video, &parse_error)) {
      if (error != NULL) {
        *error = parse_error;
      }
      return false;
    }
    msg.kind = PushMessage::kUpserted;
    msg.video = video;
    *out = msg;
    return true;
  }

  if (type == "video.removed") {
    if (!j.contains("id")) {
      if (error != NULL) {
        *error = QString::fromUtf8("video.removed missing id");
      }
      return false;
    }
    try {
      msg.removed_id = j.at("id").get<std::int64_t>();
    } catch (const std::exception& ex) {
      if (error != NULL) {
        *error = FromUtf8(ex.what());
      }
      return false;
    }
    msg.kind = PushMessage::kRemoved;
    *out = msg;
    return true;
  }

  // Unknown types are intentionally ignored.
  msg.kind = PushMessage::kIgnored;
  *out = msg;
  return true;
}

}  // namespace client
}  // namespace square_dance
