#pragma once

#include "square_dance/VideoDto.h"

#include <nlohmann/json.hpp>

#include <string>

namespace square_dance {

inline nlohmann::json videoDtoToJson(const VideoDto& video) {
  return nlohmann::json{
      {"id", video.id},
      {"external_id", video.external_id},
      {"title", video.title},
      {"play_url", video.play_url},
      {"download_url", video.download_url},
      {"source_type", video.source_type},
      {"updated_at", video.updated_at},
      {"thumbnail_url", video.thumbnail_url},
  };
}

inline std::string videoDtoToJsonString(const VideoDto& video) {
  return videoDtoToJson(video).dump();
}

inline bool videoDtoFromJson(const nlohmann::json& j,
                             VideoDto* out,
                             std::string* error) {
  if (out == NULL) {
    if (error != NULL) {
      *error = "out is null";
    }
    return false;
  }
  if (!j.is_object()) {
    if (error != NULL) {
      *error = "video json must be an object";
    }
    return false;
  }

  const char* required[] = {"id",
                            "external_id",
                            "title",
                            "play_url",
                            "download_url",
                            "source_type",
                            "updated_at"};
  for (std::size_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i) {
    if (!j.contains(required[i])) {
      if (error != NULL) {
        *error = std::string("missing field: ") + required[i];
      }
      return false;
    }
  }

  try {
    out->id = j.at("id").get<std::int64_t>();
    out->external_id = j.at("external_id").get<std::string>();
    out->title = j.at("title").get<std::string>();
    out->play_url = j.at("play_url").get<std::string>();
    out->download_url = j.at("download_url").get<std::string>();
    out->source_type = j.at("source_type").get<std::string>();
    out->updated_at = j.at("updated_at").get<std::string>();
    if (j.contains("thumbnail_url") && j.at("thumbnail_url").is_string()) {
      out->thumbnail_url = j.at("thumbnail_url").get<std::string>();
    } else {
      out->thumbnail_url.clear();
    }
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = ex.what();
    }
    return false;
  }
  return true;
}

inline bool videoDtoFromJson(const std::string& text,
                             VideoDto* out,
                             std::string* error) {
  try {
    const nlohmann::json j = nlohmann::json::parse(text);
    return videoDtoFromJson(j, out, error);
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = ex.what();
    }
    return false;
  }
}

}  // namespace square_dance
