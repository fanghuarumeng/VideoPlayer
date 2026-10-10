# Implementation Plan: Square Dance App (v1)

## Overview

Build a split-process product: a **Qt-free C++11 backend** (`SquareDanceServer`) that ingests videos from Archive.org + an authorized catalog, stores them in SQLite, probes URL health, and exposes HTTP search + websocketpp push; and a **Windows Qt5 frontend** (`SquareDanceApp`) for elderly users to search by name, play (no auto-download), and explicitly download, while receiving live catalog updates.

Specs: root `CAPABILITY-MAP.md` + `SPEC-*.md` (approved).

## Architecture Decisions

- **Two CMake targets** in one repo (`BUILD_SERVER` / `BUILD_CLIENT`); server CI must configure without Qt.
- **Backend deps:** cpp-httplib, websocketpp (+ standalone Asio preferred), nlohmann/json, sqlite3, GoogleTest — **vendored under `third_party/`** (corporate proxy blocked FetchContent to GitHub).
- **Shared contract:** `src/common/` Qt-free DTOs + JSON field names; client adapts to Qt types at the boundary.
- **HTTP `:8080` + WS `:8081`** on `127.0.0.1` for v1.
- **Media:** clients open `play_url` / `download_url` directly; server never proxies bytes.
- **Scheduling:** in-process `std::thread` timers (12h ingest, 12h health); threshold 3 deletes.
- **Vertical delivery:** first thin path = authorized catalog → SQLite → HTTP search → WS upsert → client list; then Archive.org, health, playback, download, elder polish.

## Dependency Graph

```
server-shell + common DTO
    │
    ├── video-store (SQLite)
    │       │
    │       ├── video-source (authorized → archive_org)
    │       │       │
    │       │       └── ingest (timer + runOnce + handlers)
    │       │
    │       ├── url-health (probe + delete + handlers)
    │       │
    │       └── server-api (HTTP + websocketpp) ← handlers from ingest/health
    │
client-shell
    │
    └── client-api (HTTP + Qt WebSockets)
            │
            ├── playback
            ├── download
            └── elder-ui
```

## Task List (index)

Full acceptance/verification detail lives in `tasks/todo.md`.

### Phase A — Backend foundation
- Task 1: Repo CMake + `src/common` VideoDto/JSON helpers
- Task 2: `server-shell` executable + config + GTest (no Qt)
- Task 3: `video-store` SQLite repository + tests
- **Checkpoint A**

### Phase B — First catalog path (authorized → API)
- Task 4: `AuthorizedCatalogSource` + fixtures
- Task 5: `IngestService` (runOnce + 12h timer + upsert handler)
- Task 6: HTTP API search/get + ingest trigger
- Task 7: websocketpp push server wired to ingest handler
- **Checkpoint B** — curl search + WS sees upsert

### Phase C — Backend completeness
- Task 8: `ArchiveOrgSource` (fixture parse + live optional)
- Task 9: `UrlHealthService` + remove push
- Task 10: Wire `ServerApp` main loop (HTTP+WS+timers)
- **Checkpoint C** — server-only E2E

### Phase D — Frontend foundation + search
- Task 11: `client-shell` Qt app + client config + GTest
- Task 12: `CatalogClient` HTTP search + `CatalogModel`
- Task 13: `PushClient` WebSocket + model apply rules
- **Checkpoint D** — UI lists server data + live push

### Phase E — Play / download / elder UX
- Task 14: `PlaybackController` (no download side effects)
- Task 15: `DownloadService` (explicit only)
- Task 16: `ElderMainWindow` integrate search/play/download/status
- **Checkpoint E** — product v1 manual acceptance (accepted 2026-10-10; playback quality polish deferred)

### Phase F — Polish / CI (post-v1)
- Task 17: Server CI without Qt (`scripts/ci-server.*` + GitHub Actions)
- Deferred: playback A/V sync / embedded UX polish

## Parallelization

| After | Can parallelize |
|---|---|
| Checkpoint A | Task 4 vs early client Task 11 (client blocked on API contract only — DTO already in Task 1) |
| Checkpoint B | Task 8 and Task 9 |
| Checkpoint D | Task 14 and Task 15 |

Must stay sequential: 2→3→5→6→7; 12→13→16.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| websocketpp + Asio/MSVC integration pain | High | Spike in Task 7; fall back to Boost.Asio only if standalone fails (Ask first was standalone preferred) |
| Qt Multimedia codec gaps on Windows | Med | Task 14 verify with authorized mp4 sample; document plugin need |
| Archive.org rate limits / sparse 广场舞 | Med | Authorized catalog carries product content; Archive.org proves public ingest |
| cpp-httplib + websocketpp dual listen threads | Med | Clear thread ownership in Task 10; no shared sock across libs |
| C++11 + modern header-only libs | Med | Pin versions in FetchContent; forbid C++14/17 features in our code |

## Open Questions (non-blocking defaults)

- Default fonts 18/16; no elder-facing「立即同步」(advanced menu only if added later).
- Ingest keywords: `广场舞`, `square dance`.
- Standalone Asio for websocketpp unless build forces Boost.

## Human gate

Do not start implementation until this plan / `tasks/todo.md` is approved (or you explicitly say “start Task N”).
