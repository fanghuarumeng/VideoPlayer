#pragma once

#include "video_player/server/video_store/IVideoRepository.h"

#include <string>

struct sqlite3;

namespace video_player {
namespace server {

class SqliteVideoRepository : public IVideoRepository {
 public:
  SqliteVideoRepository();
  virtual ~SqliteVideoRepository();

  bool Open(const std::string& path, std::string* error);
  void Close();

  virtual bool upsert(const Video& video, std::string* error);
  virtual std::vector<Video> searchByName(const std::string& query, int limit);
  virtual bool getById(std::int64_t id, Video* out, std::string* error);
  virtual bool findBySourceKey(const std::string& source_type,
                               const std::string& external_id,
                               Video* out,
                               std::string* error);
  virtual bool removeById(std::int64_t id, std::string* error);
  virtual std::vector<Video> listAll(int limit, int offset);

 private:
  bool EnsureSchema(std::string* error);

  sqlite3* db_;
};

}  // namespace server
}  // namespace video_player
