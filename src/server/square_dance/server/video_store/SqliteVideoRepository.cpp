#include "square_dance/server/video_store/SqliteVideoRepository.h"

#include "sqlite3.h"

#include <sstream>

namespace square_dance {
namespace server {
namespace {

void SetError(std::string* error, const std::string& message) {
  if (error != NULL) {
    *error = message;
  }
}

bool ExecSql(sqlite3* db, const char* sql, std::string* error) {
  char* errmsg = NULL;
  const int rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
  if (rc != SQLITE_OK) {
    std::string msg = "sqlite exec failed";
    if (errmsg != NULL) {
      msg += ": ";
      msg += errmsg;
      sqlite3_free(errmsg);
    }
    SetError(error, msg);
    return false;
  }
  return true;
}

Video ReadVideoRow(sqlite3_stmt* stmt) {
  Video v;
  v.id = sqlite3_column_int64(stmt, 0);
  const unsigned char* external_id = sqlite3_column_text(stmt, 1);
  const unsigned char* title = sqlite3_column_text(stmt, 2);
  const unsigned char* play_url = sqlite3_column_text(stmt, 3);
  const unsigned char* download_url = sqlite3_column_text(stmt, 4);
  const unsigned char* source_type = sqlite3_column_text(stmt, 5);
  v.consecutive_failures = sqlite3_column_int(stmt, 6);
  const unsigned char* created_at = sqlite3_column_text(stmt, 7);
  const unsigned char* updated_at = sqlite3_column_text(stmt, 8);

  v.external_id = external_id ? reinterpret_cast<const char*>(external_id) : "";
  v.title = title ? reinterpret_cast<const char*>(title) : "";
  v.play_url = play_url ? reinterpret_cast<const char*>(play_url) : "";
  v.download_url =
      download_url ? reinterpret_cast<const char*>(download_url) : "";
  v.source_type = source_type ? reinterpret_cast<const char*>(source_type) : "";
  v.created_at = created_at ? reinterpret_cast<const char*>(created_at) : "";
  v.updated_at = updated_at ? reinterpret_cast<const char*>(updated_at) : "";
  return v;
}

}  // namespace

SqliteVideoRepository::SqliteVideoRepository() : db_(NULL) {}

SqliteVideoRepository::~SqliteVideoRepository() {
  Close();
}

bool SqliteVideoRepository::Open(const std::string& path, std::string* error) {
  Close();
  const int rc = sqlite3_open(path.c_str(), &db_);
  if (rc != SQLITE_OK) {
    std::string msg = "sqlite3_open failed";
    if (db_ != NULL) {
      msg += ": ";
      msg += sqlite3_errmsg(db_);
      sqlite3_close(db_);
      db_ = NULL;
    }
    SetError(error, msg);
    return false;
  }
  if (!ExecSql(db_, "PRAGMA foreign_keys = ON;", error)) {
    Close();
    return false;
  }
  if (!EnsureSchema(error)) {
    Close();
    return false;
  }
  return true;
}

void SqliteVideoRepository::Close() {
  if (db_ != NULL) {
    sqlite3_close(db_);
    db_ = NULL;
  }
}

bool SqliteVideoRepository::EnsureSchema(std::string* error) {
  const char* kSchema =
      "CREATE TABLE IF NOT EXISTS schema_version ("
      "  version INTEGER NOT NULL"
      ");"
      "CREATE TABLE IF NOT EXISTS videos ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  external_id TEXT NOT NULL,"
      "  title TEXT NOT NULL,"
      "  play_url TEXT NOT NULL,"
      "  download_url TEXT NOT NULL,"
      "  source_type TEXT NOT NULL,"
      "  consecutive_failures INTEGER NOT NULL DEFAULT 0,"
      "  created_at TEXT NOT NULL,"
      "  updated_at TEXT NOT NULL,"
      "  UNIQUE(source_type, external_id)"
      ");";

  if (!ExecSql(db_, kSchema, error)) {
    return false;
  }

  sqlite3_stmt* stmt = NULL;
  const int prep =
      sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM schema_version;", -1, &stmt,
                         NULL);
  if (prep != SQLITE_OK) {
    SetError(error, sqlite3_errmsg(db_));
    return false;
  }
  int count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);

  if (count == 0) {
    if (!ExecSql(db_, "INSERT INTO schema_version(version) VALUES (1);",
                 error)) {
      return false;
    }
  }
  return true;
}

bool SqliteVideoRepository::upsert(const Video& video, std::string* error) {
  if (db_ == NULL) {
    SetError(error, "repository not open");
    return false;
  }

  const char* sql =
      "INSERT INTO videos("
      "  external_id, title, play_url, download_url, source_type,"
      "  consecutive_failures, created_at, updated_at"
      ") VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
      "ON CONFLICT(source_type, external_id) DO UPDATE SET "
      "  title=excluded.title,"
      "  play_url=excluded.play_url,"
      "  download_url=excluded.download_url,"
      "  consecutive_failures=excluded.consecutive_failures,"
      "  updated_at=excluded.updated_at;";

  sqlite3_stmt* stmt = NULL;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) {
    SetError(error, sqlite3_errmsg(db_));
    return false;
  }

  sqlite3_bind_text(stmt, 1, video.external_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, video.title.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, video.play_url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, video.download_url.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, video.source_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 6, video.consecutive_failures);
  sqlite3_bind_text(stmt, 7, video.created_at.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 8, video.updated_at.c_str(), -1, SQLITE_TRANSIENT);

  const int rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    SetError(error, sqlite3_errmsg(db_));
    sqlite3_finalize(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

std::vector<Video> SqliteVideoRepository::searchByName(const std::string& query,
                                                       int limit) {
  std::vector<Video> out;
  if (db_ == NULL || query.empty() || limit <= 0) {
    return out;
  }

  const char* sql =
      "SELECT id, external_id, title, play_url, download_url, source_type,"
      "       consecutive_failures, created_at, updated_at "
      "FROM videos "
      "WHERE lower(title) LIKE '%' || lower(?) || '%' "
      "ORDER BY id ASC "
      "LIMIT ?;";

  sqlite3_stmt* stmt = NULL;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) {
    return out;
  }
  sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, limit);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.push_back(ReadVideoRow(stmt));
  }
  sqlite3_finalize(stmt);
  return out;
}

bool SqliteVideoRepository::getById(std::int64_t id, Video* out,
                                    std::string* error) {
  if (db_ == NULL) {
    SetError(error, "repository not open");
    return false;
  }
  if (out == NULL) {
    SetError(error, "out is null");
    return false;
  }

  const char* sql =
      "SELECT id, external_id, title, play_url, download_url, source_type,"
      "       consecutive_failures, created_at, updated_at "
      "FROM videos WHERE id = ?;";
  sqlite3_stmt* stmt = NULL;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) {
    SetError(error, sqlite3_errmsg(db_));
    return false;
  }
  sqlite3_bind_int64(stmt, 1, id);
  const int rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    SetError(error, "video not found");
    sqlite3_finalize(stmt);
    return false;
  }
  *out = ReadVideoRow(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SqliteVideoRepository::removeById(std::int64_t id, std::string* error) {
  if (db_ == NULL) {
    SetError(error, "repository not open");
    return false;
  }

  const char* sql = "DELETE FROM videos WHERE id = ?;";
  sqlite3_stmt* stmt = NULL;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) {
    SetError(error, sqlite3_errmsg(db_));
    return false;
  }
  sqlite3_bind_int64(stmt, 1, id);
  const int rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    SetError(error, sqlite3_errmsg(db_));
    sqlite3_finalize(stmt);
    return false;
  }
  const int changes = sqlite3_changes(db_);
  sqlite3_finalize(stmt);
  if (changes == 0) {
    SetError(error, "video not found");
    return false;
  }
  return true;
}

std::vector<Video> SqliteVideoRepository::listAll(int limit, int offset) {
  std::vector<Video> out;
  if (db_ == NULL || limit <= 0) {
    return out;
  }
  if (offset < 0) {
    offset = 0;
  }

  const char* sql =
      "SELECT id, external_id, title, play_url, download_url, source_type,"
      "       consecutive_failures, created_at, updated_at "
      "FROM videos "
      "ORDER BY id ASC "
      "LIMIT ? OFFSET ?;";
  sqlite3_stmt* stmt = NULL;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL) != SQLITE_OK) {
    return out;
  }
  sqlite3_bind_int(stmt, 1, limit);
  sqlite3_bind_int(stmt, 2, offset);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.push_back(ReadVideoRow(stmt));
  }
  sqlite3_finalize(stmt);
  return out;
}

}  // namespace server
}  // namespace square_dance
