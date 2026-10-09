# Capability Map: Square Dance App (v1)

**Architecture:** split processes — pure C++ backend maintains catalog; Windows Qt frontend queries/plays/downloads and receives pushes.

| Decision | Value |
|---|---|
| Backend language | C++11 **without Qt** |
| Backend HTTP/WS/JSON/DB | cpp-httplib + websocketpp + nlohmann/json + sqlite3 |
| Frontend | C++11 + Qt 5 Widgets (+ Network/WebSockets/Multimedia) |
| Sources | C: Internet Archive + AuthorizedCatalog |
| Ingest interval | 12 hours |
| URL health | delete after 3 consecutive failures |
| Query | HTTP REST search/detail |
| Push | WebSocket (new / removed video) |
| Media bytes | frontend opens real http(s) URLs directly |
| DB | SQLite on backend; repository ready for MySQL later |

| Module id | Process | Responsibility | Depends on | Spec |
|---|---|---|---|---|
| `server-shell` | backend | CMake target, entry, config, logging, GTest | — | [SPEC-server-shell.md](SPEC-server-shell.md) |
| `video-store` | backend | Repository + SQLite; name search; MySQL-ready | `server-shell` | [SPEC-video-store.md](SPEC-video-store.md) |
| `video-source` | backend | Archive.org + AuthorizedCatalog | `server-shell` | [SPEC-video-source.md](SPEC-video-source.md) |
| `ingest` | backend | Scheduled search → upsert | `video-source`, `video-store` | [SPEC-ingest.md](SPEC-ingest.md) |
| `url-health` | backend | Probe URLs; delete after 3 failures | `video-store` | [SPEC-url-health.md](SPEC-url-health.md) |
| `server-api` | backend | HTTP + WebSocket push | `video-store`, `ingest` | [SPEC-server-api.md](SPEC-server-api.md) |
| `client-shell` | frontend | Windows Qt app shell, GTest | — | [SPEC-client-shell.md](SPEC-client-shell.md) |
| `client-api` | frontend | HTTP client + WebSocket consumer | `client-shell` | [SPEC-client-api.md](SPEC-client-api.md) |
| `playback` | frontend | Play; never download | `client-api`, `client-shell` | [SPEC-playback.md](SPEC-playback.md) |
| `download` | frontend | Download only on explicit click | `client-api`, `client-shell` | [SPEC-download.md](SPEC-download.md) |
| `elder-ui` | frontend | Search / play / download / live refresh | `playback`, `download`, `client-api` | [SPEC-elder-ui.md](SPEC-elder-ui.md) |

**Build order:**

1. `server-shell` → `video-store` → `video-source` → `ingest` + `url-health` → `server-api`
2. `client-shell` → `client-api` → `playback` + `download` → `elder-ui`

## Out of scope (v1)

- Qt linkage in any backend target
- Android / iOS acceptance
- Douyin / Kuaishou / unauthorized scrapers
- Backend media proxy / transcoding
- Complex download queue, accounts, payments
- Implementation until specs are approved

## Shared

- DTO/JSON field names shared via `src/common/` (header-only, **no Qt types** in common).
