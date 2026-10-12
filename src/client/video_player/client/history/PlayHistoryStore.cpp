#include "video_player/client/history/PlayHistoryStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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

}  // namespace

PlayHistoryStore::PlayHistoryStore(const QString& path, int max_items)
    : path_(path), max_items_(max_items > 0 ? max_items : 100) {}

QString PlayHistoryStore::path() const {
  return path_;
}

qint64 PlayHistoryStore::ResumePositionMs(const PlayHistoryItem& item) {
  if (item.duration_ms > 0 &&
      item.position_ms >= item.duration_ms - 3000) {
    return 0;
  }
  if (item.position_ms < 0) {
    return 0;
  }
  return item.position_ms;
}

bool PlayHistoryStore::load(QString* error) {
  QFile file(path_);
  if (!file.exists()) {
    items_.clear();
    return true;
  }
  if (!file.open(QIODevice::ReadOnly)) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to open history: ") + path_;
    }
    return false;
  }
  const QByteArray bytes = file.readAll();
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(bytes.constData(),
                              bytes.constData() + bytes.size());
  } catch (const std::exception& ex) {
    if (error != NULL) {
      *error = FromUtf8(ex.what());
    }
    return false;
  }
  items_.clear();
  if (!j.is_object() || !j.contains("items") || !j.at("items").is_array()) {
    return true;
  }
  const nlohmann::json& arr = j.at("items");
  for (nlohmann::json::const_iterator it = arr.begin(); it != arr.end(); ++it) {
    if (!it->is_object()) {
      continue;
    }
    PlayHistoryItem item;
    if (it->contains("id") && (*it)["id"].is_number_integer()) {
      item.id = (*it)["id"].get<std::int64_t>();
    }
    if (it->contains("title") && (*it)["title"].is_string()) {
      item.title = FromUtf8((*it)["title"].get<std::string>());
    }
    if (it->contains("play_url") && (*it)["play_url"].is_string()) {
      item.play_url = FromUtf8((*it)["play_url"].get<std::string>());
    }
    if (it->contains("thumbnail_url") && (*it)["thumbnail_url"].is_string()) {
      item.thumbnail_url = FromUtf8((*it)["thumbnail_url"].get<std::string>());
    }
    if (it->contains("position_ms") && (*it)["position_ms"].is_number_integer()) {
      item.position_ms = (*it)["position_ms"].get<std::int64_t>();
    }
    if (it->contains("duration_ms") && (*it)["duration_ms"].is_number_integer()) {
      item.duration_ms = (*it)["duration_ms"].get<std::int64_t>();
    }
    if (it->contains("updated_at") && (*it)["updated_at"].is_string()) {
      item.updated_at = FromUtf8((*it)["updated_at"].get<std::string>());
    }
    if (item.id != 0 && !item.play_url.isEmpty()) {
      items_.push_back(item);
    }
  }
  return true;
}

bool PlayHistoryStore::save(QString* error) const {
  nlohmann::json arr = nlohmann::json::array();
  for (int i = 0; i < items_.size(); ++i) {
    const PlayHistoryItem& item = items_[i];
    arr.push_back(nlohmann::json{
        {"id", item.id},
        {"title", ToUtf8(item.title)},
        {"play_url", ToUtf8(item.play_url)},
        {"thumbnail_url", ToUtf8(item.thumbnail_url)},
        {"position_ms", item.position_ms},
        {"duration_ms", item.duration_ms},
        {"updated_at", ToUtf8(item.updated_at)},
    });
  }
  nlohmann::json root;
  root["items"] = arr;

  const QFileInfo info(path_);
  QDir().mkpath(info.absolutePath());
  QFile file(path_);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to write history: ") + path_;
    }
    return false;
  }
  const std::string text = root.dump(2);
  file.write(text.data(), static_cast<qint64>(text.size()));
  return true;
}

void PlayHistoryStore::recordProgress(const Video& video,
                                      qint64 position_ms,
                                      qint64 duration_ms) {
  if (video.id == 0 || video.play_url.isEmpty()) {
    return;
  }
  for (int i = 0; i < items_.size(); ++i) {
    if (items_[i].id == video.id) {
      items_.removeAt(i);
      break;
    }
  }
  PlayHistoryItem item;
  item.id = video.id;
  item.title = video.title;
  item.play_url = video.play_url;
  item.thumbnail_url = video.thumbnail_url;
  item.position_ms = position_ms < 0 ? 0 : position_ms;
  item.duration_ms = duration_ms < 0 ? 0 : duration_ms;
  item.updated_at =
      QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  items_.prepend(item);
  while (items_.size() > max_items_) {
    items_.removeLast();
  }
  QString error;
  save(&error);
}

void PlayHistoryStore::clear() {
  items_.clear();
  QString error;
  save(&error);
}

QVector<PlayHistoryItem> PlayHistoryStore::items() const {
  return items_;
}

}  // namespace client
}  // namespace video_player
