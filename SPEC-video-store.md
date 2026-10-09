# Spec: video-store

## Objective

Backend-only persistence (pure C++ / sqlite3). Name search for HTTP API. Interface allows future MySQL without rewriting callers.

**Success:** upsert, searchByName, getById, removeById work; hard delete for url-health.

## Tech Stack

- C++11, sqlite3 C API wrapped in C++
- No Qt

## Commands

```
ctest --test-dir build -C Release -R video_store --output-on-failure
```

## Project Structure

```
src/server/video_store/
  Video.h
  IVideoRepository.h
  SqliteVideoRepository.*
tests/server/video_store/
  sqlite_repository_test.cpp
```

## Code Style

```cpp
struct Video {
  int64_t id;
  std::string external_id;
  std::string title;
  std::string play_url;
  std::string download_url;
  std::string source_type;  // "archive_org" | "authorized"
  int consecutive_failures;
  std::string created_at;   // ISO-8601 UTC string
  std::string updated_at;
};

class IVideoRepository {
 public:
  virtual ~IVideoRepository() {}
  virtual bool upsert(const Video& video, std::string* error) = 0;
  virtual std::vector<Video> searchByName(const std::string& query, int limit) = 0;
  virtual bool getById(int64_t id, Video* out, std::string* error) = 0;
  virtual bool removeById(int64_t id, std::string* error) = 0;
  virtual std::vector<Video> listAll(int limit, int offset) = 0;
};
```

- Unique `(source_type, external_id)`; empty query → empty vector; `schema_version` table.

## Testing Strategy

- Temp DB file per test; CRUD + Chinese/ASCII substring search.

## Boundaries

- **Always:** SQL behind interface; hard delete; no HTTP.
- **Ask first:** MySQL impl; soft-delete.
- **Never:** Qt types; used from frontend process.

## Success Criteria

- [ ] GTest CRUD/search green.
- [ ] Headers free of sqlite3 types where practical (`IVideoRepository` has none).
- [ ] Physical delete works.

## Open Questions

- None.
