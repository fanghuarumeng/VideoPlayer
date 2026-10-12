#include "video_player/client/api/catalog_json.h"

#include "video_player/VideoDto.h"
#include "video_player/video_json.h"

#include <nlohmann/json.hpp>

#include <string>

namespace video_player {
namespace client {
namespace {

QString FromUtf8(const std::string& s) {
  return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

std::string ToUtf8(const QString& s) {
  const QByteArray bytes = s.toUtf8();
  return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

Video FromDto(const VideoDto& dto) {
  Video v;
  v.id = dto.id;
  v.external_id = FromUtf8(dto.external_id);
  v.title = FromUtf8(dto.title);
  v.play_url = FromUtf8(dto.play_url);
  v.download_url = FromUtf8(dto.download_url);
  v.source_type = FromUtf8(dto.source_type);
  v.updated_at = FromUtf8(dto.updated_at);
  v.thumbnail_url = FromUtf8(dto.thumbnail_url);
  return v;
}

VideoDto ToDto(const Video& v) {
  VideoDto dto;
  dto.id = v.id;
  dto.external_id = ToUtf8(v.external_id);
  dto.title = ToUtf8(v.title);
  dto.play_url = ToUtf8(v.play_url);
  dto.download_url = ToUtf8(v.download_url);
  dto.source_type = ToUtf8(v.source_type);
  dto.updated_at = ToUtf8(v.updated_at);
  dto.thumbnail_url = ToUtf8(v.thumbnail_url);
  return dto;
}

}  // namespace

bool ParseVideoJson(const QByteArray& json, Video* out, QString* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = QString::fromUtf8("out is null");
    }
    return false;
  }
  VideoDto dto;
  std::string err;
  const std::string text(json.constData(), static_cast<std::size_t>(json.size()));
  if (!videoDtoFromJson(text, &dto, &err)) {
    if (error != NULL) {
      *error = FromUtf8(err);
    }
    return false;
  }
  *out = FromDto(dto);
  return true;
}

bool ParseVideoListJson(const QByteArray& json,
                        QVector<Video>* out,
                        QString* error) {
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

  if (!j.is_array()) {
    if (error != NULL) {
      *error = QString::fromUtf8("search response must be a JSON array");
    }
    return false;
  }

  QVector<Video> videos;
  videos.reserve(static_cast<int>(j.size()));
  for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it) {
    VideoDto dto;
    std::string err;
    if (!videoDtoFromJson(*it, &dto, &err)) {
      if (error != NULL) {
        *error = FromUtf8(err);
      }
      return false;
    }
    videos.push_back(FromDto(dto));
  }
  *out = videos;
  return true;
}

QByteArray VideoToJsonBytes(const Video& video) {
  const std::string text = videoDtoToJsonString(ToDto(video));
  return QByteArray(text.data(), static_cast<int>(text.size()));
}

}  // namespace client
}  // namespace video_player
