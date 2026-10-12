# Implementation Plan: Square Dance App (v1)

## Overview

Build a split-process product: a **Qt-free C++11 backend** (`VideoPlayerServer`) that ingests videos from Archive.org + an authorized catalog, stores them in SQLite, probes URL health, and exposes HTTP search + websocketpp push; and a **Windows Qt5 frontend** (`VideoPlayerApp`) for elderly users to search by name, play (no auto-download), and explicitly download, while receiving live catalog updates.

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
- Task 17: Server CI without Qt (`scripts/ci-server.*` + GitHub Actions) — done
- Task 18: In-process libav playback (`FfmpegPlaybackEngine`) — done; human motion/audio check open
- Task 19: Remote + verify GitHub Actions — next (optional parallel with G)

### Phase G — Personal media library (approved specs)
Specs: `CAPABILITY-MAP-personal-media-library.md`, `SPEC-local-media.md`, `SPEC-playlist.md`, `SPEC-library-ui.md`, `SPEC-play-history.md`.

**Architecture deltas vs Phase E:**
- Server **does** store and serve local upload bytes + thumbnails under `/media/*` (exception to “no media proxy” for `local_upload` only).
- Upload concurrency: `upload_workers` + `503 upload_busy`; thumbs: FIFO queue + `thumbnail_workers`.
- Playlist + play-history: **client-local JSON only**.

**Dependency graph (G):**
```
video-store schema v2 + VideoDto.thumbnail_url
    │
    └── local-media (UploadGate → disk/DB → ThumbnailQueue → HTTP/WS + /media)
            │
            ├── playlist (local store + pane + auto-next)
            │       │
            │       └── library-ui (nav + grid + bounded upload client + batch-add)
            │
            └── play-history (local store + resume seek + history pane)
```

- Task 20: Schema v2 + DTO/`thumbnail_url` + `GET /api/v1/videos` list
- Task 21: Upload + `UploadGate` + disk store + `/media/videos` + WS upsert (thumb empty OK)
- Task 22: `ThumbnailQueue` + ffmpeg CLI + thumb URL update + second WS upsert; skip url-health for `local_upload`
- **Checkpoint G1** — curl upload/list/play URL; burst upload ≤ workers; burst thumbs ≤ workers
- Task 23: `PlaylistStore` JSON + model tests
- Task 24: `PlaylistPane` UI + wire play/auto-next/clear
- **Checkpoint G2** — local playlist survives restart; auto-next works
- Task 25: Main-window nav (库/列表/历史/搜索) + library grid + list client
- Task 26: Client upload queue (`upload_workers`-aligned) + multi-select batch-add + `.mxf` filter
- **Checkpoint G3** — upload from UI; tiles; batch-add; search demoted
- Task 27: `PlayHistoryStore` + progress recording from playback
- Task 28: `HistoryPane` + resume seek + clear history
- **Checkpoint G4** — full personal-media path accepted

## Parallelization

| After | Can parallelize |
|---|---|
| Checkpoint A | Task 4 vs early client Task 11 (client blocked on API contract only — DTO already in Task 1) |
| Checkpoint B | Task 8 and Task 9 |
| Checkpoint D | Task 14 and Task 15 |
| Checkpoint G1 | Task 23 playlist store while polishing Task 22 tests |
| Phase F Task 19 | Parallel with Phase G (no code dependency) |

Must stay sequential: 2→3→5→6→7; 12→13→16; **20→21→22**; 23→24; 25→26; 27→28. G1 before UI upload (26); G2 before relying on batch-add UX.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| websocketpp + Asio/MSVC integration pain | High | Spike in Task 7; fall back to Boost.Asio only if standalone fails (Ask first was standalone preferred) |
| Qt Multimedia codec gaps on Windows | Med | Task 14 verify with authorized mp4 sample; document plugin need |
| Archive.org rate limits / sparse 广场舞 | Med | Authorized catalog carries product content; Archive.org proves public ingest |
| cpp-httplib + websocketpp dual listen threads | Med | Clear thread ownership in Task 10; no shared sock across libs |
| C++11 + modern header-only libs | Med | Pin versions in FetchContent; forbid C++14/17 features in our code |
| Large MXF / slow ffmpeg thumbs | Med | Async thumb queue; UI placeholder until WS thumb upsert |
| Upload 503 under burst | Low | Client local queue + backoff per `SPEC-library-ui` |
| httplib may buffer body before 503 | Med | Acquire upload slot as early as handler allows; document behavior in Task 21 |

## Open Questions (non-blocking defaults)

- Default fonts 18/16; no elder-facing「立即同步」(advanced menu only if added later).
- Ingest keywords: `广场舞`, `square dance`.
- Standalone Asio for websocketpp unless build forces Boost.

## Human gate

Phase A–E complete. Phase G specs approved 2026-10-10.  
Do not start Phase G implementation until this plan / Task 20+ in `tasks/todo.md` is approved (or you explicitly say “start Task N”).
