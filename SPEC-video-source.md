# Spec: video-source

## Objective

Pure C++ adapters returning real http(s) URLs for ingest: Archive.org + AuthorizedCatalog JSON.

## Tech Stack

- C++11, cpp-httplib (client), nlohmann/json
- No Qt

## Commands

```
ctest --test-dir build -C Release -R video_source --output-on-failure
```

## Project Structure

```
src/server/video_source/
  VideoCandidate.h
  IVideoSource.h
  ArchiveOrgSource.*
  AuthorizedCatalogSource.*
tests/server/video_source/
  archive_org_parse_test.cpp
  authorized_catalog_test.cpp
```

## Code Style

```cpp
struct VideoCandidate {
  std::string external_id;
  std::string title;
  std::string play_url;
  std::string download_url;
  std::string source_type;
};

class IVideoSource {
 public:
  virtual ~IVideoSource() {}
  virtual std::string id() const = 0;
  virtual std::vector<VideoCandidate> search(const std::string& keyword,
                                             int limit,
                                             std::string* error) = 0;
};
```

Authorized catalog JSON unchanged (see prior examples). Archive.org: advancedsearch + `/metadata/{id}` → `/download/{id}/{file}`; prefer mp4 when present.

## Testing Strategy

- Fixture-based parse tests; CI without live network.

## Boundaries

- **Always:** http(s) only; no banned short-video scrapers.
- **Ask first:** third adapter.
- **Never:** write DB; download media files; Qt includes.

## Success Criteria

- [ ] Both adapters implement `IVideoSource`.
- [ ] Fixture URL resolution tests pass.
- [ ] Malformed authorized entries rejected.

## Open Questions

- Default keywords: `广场舞`, `square dance`.
