# Task List: Square Dance App (v1)

Track progress here. Specs: `CAPABILITY-MAP.md`, `SPEC-*.md`. Plan: `tasks/plan.md`.

---

## Task 1: CMake root + common DTO

**Description:** Create root CMake project with `BUILD_SERVER`/`BUILD_CLIENT` options, FetchContent pins for server deps (nlohmann/json, googletest; httplib/websocketpp/sqlite3 as needed by later tasks), and Qt-free `src/common` VideoDto + JSON field helpers shared by both processes.

**Acceptance criteria:**
- [x] CMake configures with `-DBUILD_SERVER=ON -DBUILD_CLIENT=OFF` without Qt
- [x] `VideoDto` + to/from JSON helpers compile in a tiny common test or are header-only and included by a server test target stub
- [x] JSON field names match `SPEC-server-api.md`

**Verification:**
- [x] `cmake -S . -B build-server -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF` succeeds without `CMAKE_PREFIX_PATH` for Qt
- [x] Manual check: `src/common` has no `#include <Q...>`

**Status:** Done (2026-10-09). Deps vendored under `third_party/` (FetchContent to GitHub blocked by proxy).

**Dependencies:** None  
**Files likely touched:** `CMakeLists.txt`, `cmake/*`, `src/common/VideoDto.h`, `src/common/video_json.h`  
**Estimated scope:** M

---

## Task 2: server-shell

**Description:** Add `SquareDanceServer` executable: `main`, `ServerConfig` JSON load, logging, GoogleTest target for config parse. No business modules yet.

**Acceptance criteria:**
- [x] `SquareDanceServer` builds and runs `--config` / example config
- [x] Config fields match `SPEC-server-shell.md` (ports, paths, intervals, keywords, threshold)
- [x] Server targets do not link Qt
- [x] Config GTest passes

**Verification:**
- [x] `cmake --build build-server --config Release --target SquareDanceServer`
- [x] `ctest --test-dir build-server -C Release -R ServerConfig --output-on-failure`
- [x] Manual: process starts and exits cleanly

**Dependencies:** Task 1  
**Files likely touched:** `src/server/main.cpp`, `ServerConfig.*`, `config/server.json.example`, `tests/server/server_config_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Shell loads config and exits; listen/API wired in later tasks.

---

## Task 3: video-store (SQLite)

**Description:** Implement `IVideoRepository` + `SqliteVideoRepository` with schema version, upsert uniqueness, searchByName, hard delete.

**Acceptance criteria:**
- [ ] Upsert insert + update by `(source_type, external_id)`
- [ ] `searchByName` substring; empty query → empty list
- [ ] `removeById` physically deletes
- [ ] GTest suite green on temp DB

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R video_store --output-on-failure`

**Dependencies:** Task 2  
**Files likely touched:** `src/server/video_store/*`, `tests/server/video_store/sqlite_repository_test.cpp`  
**Estimated scope:** M

---

## Checkpoint A — Backend foundation

- [ ] Server configures/builds without Qt
- [ ] video_store tests pass
- [ ] Review with human before Phase B

---

## Task 4: AuthorizedCatalogSource

**Description:** Implement authorized JSON catalog adapter; reject malformed entries; filter by keyword.

**Acceptance criteria:**
- [ ] Loads catalog from path; returns `VideoCandidate`s with real http(s) URLs
- [ ] Keyword filter on title; missing URL entries skipped/rejected
- [ ] Fixture GTest without network

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R authorized_catalog --output-on-failure`

**Dependencies:** Task 2 (Task 3 not strictly required but keep order)  
**Files likely touched:** `src/server/video_source/IVideoSource.h`, `AuthorizedCatalogSource.*`, `tests/server/video_source/authorized_catalog_test.cpp`, sample `config/authorized_catalog.example.json`  
**Estimated scope:** M

---

## Task 5: IngestService

**Description:** `runOnce` over configured sources → upsert into repository; 12h timer; upserted handler callback; no media download.

**Acceptance criteria:**
- [ ] Double `runOnce` does not duplicate rows
- [ ] Handler fires with inserted/updated videos (skip identical no-op when detectable)
- [ ] Timer start/stop safe
- [ ] GTest with fake source + temp repo

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R ingest --output-on-failure`

**Dependencies:** Task 3, Task 4  
**Files likely touched:** `src/server/ingest/*`, `tests/server/ingest/ingest_service_test.cpp`  
**Estimated scope:** M

---

## Task 6: HTTP API (search / get / ingest run)

**Description:** cpp-httplib server exposing search, get-by-id, POST ingest/run; JSON via common helpers.

**Acceptance criteria:**
- [ ] `GET /api/v1/videos/search` returns matching videos
- [ ] `GET /api/v1/videos/{id}` 200/404
- [ ] `POST /api/v1/ingest/run` triggers `runOnce`
- [ ] Ephemeral-port GTest or equivalent

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R server_api --output-on-failure` (or `http_search`)
- [ ] Manual: `curl` against running server after Checkpoint B wiring

**Dependencies:** Task 3, Task 5  
**Files likely touched:** `src/server/api/HttpApiServer.*`, `video_json.*`, `tests/server/api/http_search_test.cpp`  
**Estimated scope:** M

---

## Task 7: websocketpp push server

**Description:** WS server on `/ws` port 8081; broadcast `video.upserted` / `video.removed`; connect ingest handler to upsert push.

**Acceptance criteria:**
- [ ] Client can connect to `ws://127.0.0.1:8081/ws`
- [ ] Ingest-driven upsert produces `video.upserted` JSON
- [ ] Unknown client messages ignored; ping optional
- [ ] Builds with websocketpp + Asio (standalone preferred)

**Verification:**
- [ ] Automated test if feasible; else manual WS client script documented in task notes
- [ ] Build Release `SquareDanceServer` succeeds

**Dependencies:** Task 5, Task 6  
**Files likely touched:** `src/server/api/WsPushServer.*`, CMake FetchContent for websocketpp/asio, tests or `tools/ws_smoke.*`  
**Estimated scope:** M

---

## Checkpoint B — Authorized → HTTP → WS

- [ ] Ingest from example authorized catalog fills DB
- [ ] curl search returns titles
- [ ] WS client sees upsert after ingest
- [ ] Human review before Archive.org / health

---

## Task 8: ArchiveOrgSource

**Description:** Archive.org search + metadata → direct download URL; fixture-based parse tests; wire into ingest source list.

**Acceptance criteria:**
- [ ] Fixture tests resolve a sample identifier to download URL
- [ ] Prefer mp4 when present; skip items without usable file
- [ ] Live network not required for CI green

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R archive_org --output-on-failure`

**Dependencies:** Task 4, Task 5  
**Files likely touched:** `src/server/video_source/ArchiveOrgSource.*`, `tests/server/video_source/archive_org_parse_test.cpp`, fixtures under `tests/server/video_source/fixtures/`  
**Estimated scope:** M

---

## Task 9: UrlHealthService

**Description:** Probe play/download URLs; after 3 failures hard-delete; removed handler for WS.

**Acceptance criteria:**
- [ ] Mock prober: 3 fails → deleted + handler; success resets counter
- [ ] 12h timer start/stop
- [ ] HEAD with GET fallback behavior covered or documented in prober

**Verification:**
- [ ] `ctest --test-dir build-server -C Release -R url_health --output-on-failure`

**Dependencies:** Task 3  
**Files likely touched:** `src/server/url_health/*`, `tests/server/url_health/url_health_service_test.cpp`  
**Estimated scope:** M

---

## Task 10: ServerApp wiring

**Description:** Compose config, store, sources, ingest, health, HTTP, WS in `ServerApp`; single process entry runs all; health removals push WS.

**Acceptance criteria:**
- [ ] One `SquareDanceServer` process serves HTTP+WS and runs timers
- [ ] Health deletion emits `video.removed`
- [ ] Example configs + authorized catalog documented for local run

**Verification:**
- [ ] Manual E2E: start server → ingest → search → kill URL in DB simulation or mock → removed event
- [ ] All server ctests still green

**Dependencies:** Task 6, Task 7, Task 8, Task 9  
**Files likely touched:** `src/server/ServerApp.*`, `src/server/main.cpp`, `config/*`  
**Estimated scope:** M

---

## Checkpoint C — Backend E2E

- [ ] Server-only flow demos search + push + health delete
- [ ] No Qt linked into server
- [ ] Human review before client work

---

## Task 11: client-shell

**Description:** `SquareDanceApp` Qt5 Widgets target, `ClientConfig`, empty `MainWindow`, client GTest smoke. No SQLite.

**Acceptance criteria:**
- [ ] App opens a window with `--config config/client.json`
- [ ] Config has `http_base_url`, `ws_url`, `download_dir`
- [ ] Client smoke GTest passes
- [ ] No QtSql in client link line

**Verification:**
- [ ] Configure with Qt `CMAKE_PREFIX_PATH`; build `SquareDanceApp`
- [ ] `ctest --test-dir build-client -C Release -R client_config --output-on-failure`
- [ ] Manual: window visible

**Dependencies:** Task 1  
**Files likely touched:** `src/client/main.cpp`, `ClientConfig.*`, `MainWindow.*`, `config/client.json.example`, `tests/client/*`  
**Estimated scope:** M

---

## Task 12: CatalogClient + CatalogModel

**Description:** HTTP search/detail client; list model for UI binding; parse common JSON.

**Acceptance criteria:**
- [ ] `search(name)` emits results or error
- [ ] Model resets from search results
- [ ] JSON parse unit test from fixture

**Verification:**
- [ ] `ctest --test-dir build-client -C Release -R catalog --output-on-failure`
- [ ] Manual against running server: search returns rows

**Dependencies:** Task 6, Task 11  
**Files likely touched:** `src/client/api/CatalogClient.*`, `CatalogModel.*`, `tests/client/api/*`  
**Estimated scope:** M

---

## Task 13: PushClient

**Description:** Qt WebSockets client; reconnect backoff; apply upsert/remove to `CatalogModel` per elder-ui filter rules.

**Acceptance criteria:**
- [ ] Connects to `ws_url`; handles `video.upserted` / `video.removed`
- [ ] Connection state signal for UI
- [ ] Ignores unknown message types

**Verification:**
- [ ] Manual: server ingest → client model updates without re-search
- [ ] Unit test for JSON message dispatch if practical

**Dependencies:** Task 7, Task 12  
**Files likely touched:** `src/client/api/PushClient.*`, model update helpers  
**Estimated scope:** M

---

## Checkpoint D — Client catalog live

- [ ] Client search works against local server
- [ ] WS push updates list
- [ ] Human review before play/download

---

## Task 14: Playback

**Description:** `PlaybackController` + video widget; play/pause/stop from `play_url`; must not write download dir.

**Acceptance criteria:**
- [ ] Play selected video URL
- [ ] Play path does not call download APIs / does not create files in `download_dir`
- [ ] Error signal on empty/bad URL

**Verification:**
- [ ] Test or script asserting download_dir unchanged after play-only
- [ ] Manual play of authorized sample mp4

**Dependencies:** Task 12  
**Files likely touched:** `src/client/playback/*`, `tests/client/playback/*`  
**Estimated scope:** M

---

## Task 15: Download

**Description:** Explicit `DownloadService` to `download_dir`; progress/finished/error; cancel cleans partial.

**Acceptance criteria:**
- [ ] User-triggered download creates local file
- [ ] Not invoked from playback
- [ ] Progress signals fire

**Verification:**
- [ ] `ctest` or manual download of small file URL
- [ ] Confirm playback module has no link/call to DownloadService

**Dependencies:** Task 11  
**Files likely touched:** `src/client/download/*`, `tests/client/download/*`  
**Estimated scope:** M

---

## Task 16: ElderMainWindow

**Description:** Wire search bar, large result list, play, download, connection status; Chinese labels; fonts ≥ 16/18; no elder「立即同步」.

**Acceptance criteria:**
- [ ] Name search → list → Play / Download behaviors match specs
- [ ] WS connected/disconnected visible
- [ ] Push updates list per filter rules in `SPEC-elder-ui.md`
- [ ] Manual arm’s-length readability pass

**Verification:**
- [ ] Manual checklist against running server + sample catalog
- [ ] Full client+server build green

**Dependencies:** Task 13, Task 14, Task 15  
**Files likely touched:** `src/client/elder_ui/*`, `MainWindow` integration  
**Estimated scope:** M

---

## Checkpoint E — v1 complete

- [ ] All module success criteria from approved SPECs met or explicitly deferred
- [ ] Server builds without Qt; client builds with Qt
- [ ] Play does not auto-download; download is explicit
- [ ] Human product acceptance before polish/CI extras

---

## Notes

- Prefer implementing **one task per session**; stop at checkpoints for review.
- Do not start Task N until dependencies are done and prior checkpoint (if any) is acknowledged when the plan requires human review.
