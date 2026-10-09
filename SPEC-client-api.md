# Spec: client-api

## Objective

Frontend networking layer: call backend HTTP search/detail and maintain a WebSocket that applies push events into an in-memory catalog model for the UI.

**Success:** search returns videos; WS upsert adds/updates rows in the model; WS removed drops rows; no SQLite on client.

## Tech Stack

- C++11, Qt 5 Network + **Qt5::WebSockets**
- Shared DTO/JSON field names from `src/common/` (**no Qt types** in common; client converts to `QString` at the boundary)
- Depends on: `client-shell` config
- Backend is pure C++; wire format is JSON over HTTP/WS only

## Commands

```
ctest --test-dir build -C Release -R client_api --output-on-failure
```

## Project Structure

```
src/client/api/
  CatalogClient.*       → HTTP
  PushClient.*          → WebSocket
  CatalogModel.*        → QAbstractListModel or plain cache + signals
tests/client/api/
  catalog_json_test.cpp
```

## Code Style

```cpp
class CatalogClient : public QObject {
  Q_OBJECT
 public:
  void search(const QString& name, int limit);
 signals:
  void searchFinished(const QVector<Video>& videos);
  void searchFailed(const QString& error);
};

class PushClient : public QObject {
  Q_OBJECT
 public:
  void connectToServer(const QUrl& wsUrl);
 signals:
  void videoUpserted(const Video& video);
  void videoRemoved(qint64 id);
  void connectionChanged(bool connected);
};
```

- Reconnect with backoff on WS drop.
- Parse server JSON into `square_dance::VideoDto` (`src/common`), then adapt for UI models.

## Testing Strategy

- JSON parse fixtures.
- Optional local mock HTTP server in tests.

## Boundaries

- **Always:** use config URLs; parse unknown WS `type` by ignoring.
- **Ask first:** caching search results to disk; offline mode.
- **Never:** open backend SQLite; download media inside this module.

## Success Criteria

- [ ] HTTP search wired to signals.
- [ ] WS upsert/removed update CatalogModel.
- [ ] Disconnect/reconnect does not crash UI.

## Open Questions

- None (shared `src/common/VideoDto.h` without Qt — yes).
