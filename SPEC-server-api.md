# Spec: server-api

## Objective

Pure C++ HTTP + WebSocket surface for frontends. No Qt.

- HTTP (cpp-httplib): search, get-by-id, optional ingest trigger  
- WebSocket (**websocketpp**): push `video.upserted` / `video.removed`

## Tech Stack

- C++11, cpp-httplib, websocketpp (+ Asio), nlohmann/json

## Commands

```
ctest --test-dir build -C Release -R server_api --output-on-failure
curl "http://127.0.0.1:8080/api/v1/videos/search?q=test&limit=20"
```

## Project Structure

```
src/server/api/
  HttpApiServer.*
  WsPushServer.*
  video_json.*
tests/server/api/
  video_json_test.cpp
  http_search_test.cpp
```

## Code Style

### HTTP

| Method | Path | Behavior |
|---|---|---|
| GET | `/api/v1/videos/search?q=&limit=` | searchByName |
| GET | `/api/v1/videos?limit=&offset=` | listAll newest-first (default limit 50, max 200) |
| GET | `/api/v1/videos/{id}` | detail or 404 |
| POST | `/api/v1/ingest/run` | runOnce |

JSON fields (Qt-free, shared with client):

```json
{
  "id": 1,
  "external_id": "...",
  "title": "...",
  "play_url": "https://...",
  "download_url": "https://...",
  "source_type": "archive_org",
  "updated_at": "2026-10-09T08:00:00Z",
  "thumbnail_url": ""
}
```

### WebSocket

- `ws://127.0.0.1:8081/ws`
- `{ "type": "video.upserted", "video": { ... } }`
- `{ "type": "video.removed", "id": 1 }`

Push on true inserts or title/URL changes; skip identical no-op upserts. Bind `127.0.0.1`; ports 8080/8081.

## Testing Strategy

- JSON unit tests; HTTP against temp repo on ephemeral port.

## Boundaries

- **Always:** no media proxy; no Qt; localhost trust v1 (no auth).
- **Ask first:** TLS, auth, single-port multiplex.
- **Never:** expose SQL.

## Success Criteria

- [ ] HTTP search/get work.
- [ ] WS pushes on ingest/health events.
- [ ] Demonstrable with curl + simple WS client.

## Open Questions

- None for library choice (websocketpp locked); Asio flavor follows server-shell.
