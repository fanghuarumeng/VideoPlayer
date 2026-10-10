#include "square_dance/client/playlist/PlaylistStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <nlohmann/json.hpp>

#include <string>

namespace square_dance {
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

PlaylistStore::PlaylistStore(const QString& path)
    : path_(path), current_index_(-1) {}

QString PlaylistStore::path() const {
  return path_;
}

bool PlaylistStore::ContainsId(qint64 id) const {
  for (int i = 0; i < items_.size(); ++i) {
    if (items_[i].id == id) {
      return true;
    }
  }
  return false;
}

void PlaylistStore::ClampIndex() {
  if (items_.isEmpty()) {
    current_index_ = -1;
    return;
  }
  if (current_index_ < 0) {
    current_index_ = 0;
  }
  if (current_index_ >= items_.size()) {
    current_index_ = items_.size() - 1;
  }
}

bool PlaylistStore::load(QString* error) {
  QFile file(path_);
  if (!file.exists()) {
    items_.clear();
    current_index_ = -1;
    return true;
  }
  if (!file.open(QIODevice::ReadOnly)) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to open playlist: ") + path_;
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
  if (!j.is_object()) {
    if (error != NULL) {
      *error = QString::fromUtf8("playlist root must be an object");
    }
    return false;
  }

  QVector<PlaylistItem> loaded;
  if (j.contains("items") && j.at("items").is_array()) {
    const nlohmann::json& arr = j.at("items");
    for (nlohmann::json::const_iterator it = arr.begin(); it != arr.end();
         ++it) {
      if (!it->is_object()) {
        continue;
      }
      PlaylistItem item;
      try {
        if (it->contains("id") && (*it)["id"].is_number_integer()) {
          item.id = (*it)["id"].get<std::int64_t>();
        }
        if (it->contains("title") && (*it)["title"].is_string()) {
          item.title = FromUtf8((*it)["title"].get<std::string>());
        }
        if (it->contains("play_url") && (*it)["play_url"].is_string()) {
          item.play_url = FromUtf8((*it)["play_url"].get<std::string>());
        }
        if (it->contains("thumbnail_url") &&
            (*it)["thumbnail_url"].is_string()) {
          item.thumbnail_url =
              FromUtf8((*it)["thumbnail_url"].get<std::string>());
        }
      } catch (const std::exception& ex) {
        if (error != NULL) {
          *error = FromUtf8(ex.what());
        }
        return false;
      }
      if (item.id == 0 || item.play_url.isEmpty()) {
        continue;
      }
      loaded.push_back(item);
    }
  }

  items_ = loaded;
  current_index_ = -1;
  if (j.contains("current_index") && j.at("current_index").is_number_integer()) {
    current_index_ = j.at("current_index").get<int>();
  }
  ClampIndex();
  return true;
}

bool PlaylistStore::save(QString* error) const {
  nlohmann::json arr = nlohmann::json::array();
  for (int i = 0; i < items_.size(); ++i) {
    const PlaylistItem& item = items_[i];
    arr.push_back(nlohmann::json{
        {"id", item.id},
        {"title", ToUtf8(item.title)},
        {"play_url", ToUtf8(item.play_url)},
        {"thumbnail_url", ToUtf8(item.thumbnail_url)},
    });
  }
  nlohmann::json root;
  root["current_index"] = current_index_;
  root["items"] = arr;

  const QFileInfo info(path_);
  const QString dir = info.absolutePath();
  if (!dir.isEmpty()) {
    QDir().mkpath(dir);
  }

  QFile file(path_);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to write playlist: ") + path_;
    }
    return false;
  }
  const std::string text = root.dump(2);
  if (file.write(text.data(), static_cast<qint64>(text.size())) < 0) {
    if (error != NULL) {
      *error = QString::fromUtf8("failed to write playlist bytes");
    }
    return false;
  }
  return true;
}

bool PlaylistStore::append(const PlaylistItem& item) {
  if (item.id == 0 || item.play_url.isEmpty()) {
    return false;
  }
  if (ContainsId(item.id)) {
    return false;
  }
  items_.push_back(item);
  if (current_index_ < 0) {
    current_index_ = 0;
  }
  QString error;
  save(&error);
  return true;
}

bool PlaylistStore::append(const Video& video) {
  PlaylistItem item;
  item.id = video.id;
  item.title = video.title;
  item.play_url = video.play_url;
  item.thumbnail_url = video.thumbnail_url;
  return append(item);
}

void PlaylistStore::clear() {
  items_.clear();
  current_index_ = -1;
  QString error;
  save(&error);
}

QVector<PlaylistItem> PlaylistStore::items() const {
  return items_;
}

int PlaylistStore::currentIndex() const {
  return current_index_;
}

void PlaylistStore::setCurrentIndex(int index) {
  current_index_ = index;
  ClampIndex();
  QString error;
  save(&error);
}

}  // namespace client
}  // namespace square_dance
