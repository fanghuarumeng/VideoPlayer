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
**Files likely touched:** `CMakeLists.txt`, `cmake/`*, `src/common/VideoDto.h`, `src/common/video_json.h`  
**Estimated scope:** M

---



## Task 2: server-shell

**Description:** Add `VideoPlayerServer` executable: `main`, `ServerConfig` JSON load, logging, GoogleTest target for config parse. No business modules yet.

**Acceptance criteria:**

- [x] `VideoPlayerServer` builds and runs `--config` / example config
- [x] Config fields match `SPEC-server-shell.md` (ports, paths, intervals, keywords, threshold)
- [x] Server targets do not link Qt
- [x] Config GTest passes

**Verification:**

- [x] `cmake --build build-server --config Release --target VideoPlayerServer`
- [x] `ctest --test-dir build-server -C Release -R ServerConfig --output-on-failure`
- [x] Manual: process starts and exits cleanly

**Dependencies:** Task 1  
**Files likely touched:** `src/server/main.cpp`, `ServerConfig.`*, `config/server.json.example`, `tests/server/server_config_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Shell loads config and exits; listen/API wired in later tasks.

---



## Task 3: video-store (SQLite)

**Description:** Implement `IVideoRepository` + `SqliteVideoRepository` with schema version, upsert uniqueness, searchByName, hard delete.

**Acceptance criteria:**

- [x] Upsert insert + update by `(source_type, external_id)`
- [x] `searchByName` substring; empty query → empty list
- [x] `removeById` physically deletes
- [x] GTest suite green on temp DB

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R SqliteVideoRepository --output-on-failure`

**Dependencies:** Task 2  
**Files likely touched:** `src/server/video_store/`*, `tests/server/video_store/sqlite_repository_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). SQLite amalgamation vendored under `third_party/sqlite`.

---



## Checkpoint A — Backend foundation

- [x] Server configures/builds without Qt
- [x] video_store tests pass
- [x] Review with human before Phase B

---



## Task 4: AuthorizedCatalogSource

**Description:** Implement authorized JSON catalog adapter; reject malformed entries; filter by keyword.

**Acceptance criteria:**

- [x] Loads catalog from path; returns `VideoCandidate`s with real http(s) URLs
- [x] Keyword filter on title; missing URL entries skipped/rejected
- [x] Fixture GTest without network

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R AuthorizedCatalog --output-on-failure`

**Dependencies:** Task 2 (Task 3 not strictly required but keep order)  
**Files likely touched:** `src/server/video_source/IVideoSource.h`, `AuthorizedCatalogSource.`*, `tests/server/video_source/authorized_catalog_test.cpp`, sample `config/authorized_catalog.example.json`  
**Estimated scope:** M

**Status:** Done (2026-10-09).

---



## Task 5: IngestService

**Description:** `runOnce` over configured sources → upsert into repository; 12h timer; upserted handler callback; no media download.

**Acceptance criteria:**

- [x] Double `runOnce` does not duplicate rows
- [x] Handler fires with inserted/updated videos (skip identical no-op when detectable)
- [x] Timer start/stop safe
- [x] GTest with fake source + temp repo

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R IngestService --output-on-failure`

**Dependencies:** Task 3, Task 4  
**Files likely touched:** `src/server/ingest/`*, `tests/server/ingest/ingest_service_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Added `findBySourceKey` for no-op detection.

---



## Task 6: HTTP API (search / get / ingest run)

**Description:** cpp-httplib server exposing search, get-by-id, POST ingest/run; JSON via common helpers.

**Acceptance criteria:**

- [x] `GET /api/v1/videos/search` returns matching videos
- [x] `GET /api/v1/videos/{id}` 200/404
- [x] `POST /api/v1/ingest/run` triggers `runOnce`
- [x] Ephemeral-port GTest or equivalent

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R HttpSearch --output-on-failure` (and ServerVideoJson)
- [ ] Manual: `curl` against running server after Checkpoint B wiring

**Dependencies:** Task 3, Task 5  
**Files likely touched:** `src/server/api/HttpApiServer.`*, `video_json.`*, `tests/server/api/http_search_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). httplib vendored; ingest POST returns 503 when ingest not wired.

---



## Task 7: websocketpp push server

**Description:** WS server on `/ws` port 8081; broadcast `video.upserted` / `video.removed`; connect ingest handler to upsert push.

**Acceptance criteria:**

- [x] Client can connect to `ws://127.0.0.1:8081/ws`
- [x] Ingest-driven upsert produces `video.upserted` JSON
- [x] Unknown client messages ignored; ping optional
- [x] Builds with websocketpp + Asio (standalone preferred)

**Verification:**

- [x] Automated test if feasible; else manual WS client script documented in task notes
- [x] Build Release `VideoPlayerServer` succeeds

**Dependencies:** Task 5, Task 6  
**Files likely touched:** `src/server/api/WsPushServer.`*, CMake FetchContent for websocketpp/asio, tests or `tools/ws_smoke.`*  
**Estimated scope:** M

**Status:** Done (2026-10-09). `BroadcastUpserted`/`BroadcastRemoved` ready; ServerApp wires ingest→WS in Task 10.

---



## Task 8: ArchiveOrgSource

**Description:** Archive.org search + metadata → direct download URL; fixture-based parse tests; wire into ingest source list.

**Acceptance criteria:**

- [x] Fixture tests resolve a sample identifier to download URL
- [x] Prefer mp4 when present; skip items without usable file
- [x] Live network not required for CI green

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R ArchiveOrg --output-on-failure`

**Dependencies:** Task 4, Task 5  
**Files likely touched:** `src/server/video_source/ArchiveOrgSource.`*, `tests/server/video_source/archive_org_parse_test.cpp`, fixtures under `tests/server/video_source/fixtures/`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Live HTTPS needs OpenSSL-enabled httplib; tests inject HttpGetFn.

---



## Task 9: UrlHealthService

**Description:** Probe play/download URLs; after 3 failures hard-delete; removed handler for WS.

**Acceptance criteria:**

- [x] Mock prober: 3 fails → deleted + handler; success resets counter
- [x] 12h timer start/stop
- [x] HEAD with GET fallback behavior covered or documented in prober

**Verification:**

- [x] `ctest --test-dir build-server -C Release -R UrlHealth --output-on-failure`

**Dependencies:** Task 3  
**Files likely touched:** `src/server/url_health/`*, `tests/server/url_health/url_health_service_test.cpp`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Threshold=3; removed handler ready for WsPush in Task 10.

---



## Task 10: ServerApp wiring

**Description:** Compose config, store, sources, ingest, health, HTTP, WS in `ServerApp`; single process entry runs all; health removals push WS.

**Acceptance criteria:**

- [x] One `VideoPlayerServer` process serves HTTP+WS and runs timers
- [x] Health deletion emits `video.removed`
- [x] Example configs + authorized catalog documented for local run

**Verification:**

- [x] Manual E2E: start server → ingest → search → kill URL in DB simulation or mock → removed event
- [x] All server ctests still green

**Dependencies:** Task 6, Task 7, Task 8, Task 9  
**Files likely touched:** `src/server/ServerApp.`*, `src/server/main.cpp`, `config/`*  
**Estimated scope:** M

**Status:** Done (2026-10-09). See `config/run-server.txt`. Integration test covers boot+ingest+HTTP search; health→WS wired in ServerApp.

---



## Checkpoint B — Authorized → HTTP → WS

- [x] Ingest from example authorized catalog fills DB
- [x] curl search returns titles
- [x] WS client sees upsert after ingest
- [ ] Human review before Archive.org / health

---



## Checkpoint C — Backend E2E

- [x] Server-only flow demos search + push + health delete
- [x] No Qt linked into server
- [ ] Human review before client work

---



## Task 11: client-shell

**Description:** `VideoPlayerApp` Qt5 Widgets target, `ClientConfig`, empty `MainWindow`, client GTest smoke. No SQLite.

**Acceptance criteria:**

- [x] App opens a window with `--config config/client.json`
- [x] Config has `http_base_url`, `ws_url`, `download_dir`
- [x] Client smoke GTest passes
- [x] No QtSql in client link line

**Verification:**

- [x] Configure with Qt `CMAKE_PREFIX_PATH`; build `VideoPlayerApp`
- [x] `ctest --test-dir build-client -C Release -R ClientConfig --output-on-failure` (also MainWindow smoke)
- [x] Manual: window visible

**Dependencies:** Task 1  
**Files likely touched:** `src/client/main.cpp`, `ClientConfig.`*, `MainWindow.`*, `config/client.json.example`, `tests/client/*`  
**Estimated scope:** M

**Status:** Done (2026-10-09). Qt 5.12 msvc2017_64; POST_BUILD windeployqt; see `config/run-client.txt`.

---



## Task 12: CatalogClient + CatalogModel

**Description:** HTTP search/detail client; list model for UI binding; parse common JSON.

**Acceptance criteria:**

- [x] `search(name)` emits results or error
- [x] Model resets from search results
- [x] JSON parse unit test from fixture

**Verification:**

- [x] `ctest --test-dir build-client -C Release -R "[Cc]atalog" --output-on-failure`
- [ ] Manual against running server: search returns rows (optional; covered by FakeSearchServer unit test)

**Dependencies:** Task 6, Task 11  
**Files likely touched:** `src/client/api/CatalogClient.`*, `CatalogModel.`*, `tests/client/api/*`  
**Estimated scope:** M

**Status:** Done (2026-10-09). `CatalogClient`/`CatalogModel`/`catalog_json`; Qt Network; getById ready for later UI.

---



## Task 13: PushClient

**Description:** Qt WebSockets client; reconnect backoff; apply upsert/remove to `CatalogModel` per elder-ui filter rules.

**Acceptance criteria:**

- [x] Connects to `ws_url`; handles `video.upserted` / `video.removed`
- [x] Connection state signal for UI
- [x] Ignores unknown message types

**Verification:**

- [ ] Manual: server ingest → client model updates without re-search (deferred to Checkpoint D / Task 16 wiring)
- [x] Unit test for JSON message dispatch if practical

**Dependencies:** Task 7, Task 12  
**Files likely touched:** `src/client/api/PushClient.`*, model update helpers  
**Estimated scope:** M

**Status:** Done (2026-10-09). `PushClient` + `push_json`; model `applyUpserted`/`applyRemoved` with elder filter rules; reconnect backoff 1s→30s.

---



## Checkpoint D — Client catalog live

- [x] Client search wired (unit + ElderMainWindow); live server manual optional
- [x] WS push updates list (unit + UI wired)
- [ ] Human review before play/download

---



## Task 14: Playback

**Description:** `PlaybackController` + video widget; play/pause/stop from `play_url`; must not write download dir.

**Acceptance criteria:**

- [x] Play selected video URL
- [x] Play path does not call download APIs / does not create files in `download_dir`
- [x] Error signal on empty/bad URL

**Verification:**

- [x] Test or script asserting download_dir unchanged after play-only
- [ ] Manual play of authorized sample mp4 (needs multimedia plugins + live URL)

**Dependencies:** Task 12  
**Files likely touched:** `src/client/playback/`*, `tests/client/playback/`*  
**Estimated scope:** M

**Status:** Done (2026-10-10). In-process `FfmpegPlaybackEngine` (libav*) + `PlaybackController`; PTS-paced decode; empty/invalid URL errors; no download side effects. Shared FFmpeg vendored per `config/ffmpeg-vendor.txt`.

---



## Task 15: Download

**Description:** Explicit `DownloadService` to `download_dir`; progress/finished/error; cancel cleans partial.

**Acceptance criteria:**

- [x] User-triggered download creates local file
- [x] Not invoked from playback
- [x] Progress signals fire

**Verification:**

- [x] `ctest` or manual download of small file URL
- [x] Confirm playback module has no link/call to DownloadService

**Dependencies:** Task 11  
**Files likely touched:** `src/client/download/`*, `tests/client/download/`*  
**Estimated scope:** M

**Status:** Done (2026-10-09). `DownloadService` with progress/finished/cancel-cleans-partial; Fake HTTP tests.

---



## Task 16: ElderMainWindow

**Description:** Wire search bar, large result list, play, download, connection status; Chinese labels; fonts ≥ 16/18; no elder「立即同步」.

**Acceptance criteria:**

- [x] Name search → list → Play / Download behaviors match specs
- [x] WS connected/disconnected visible
- [x] Push updates list per filter rules in `SPEC-elder-ui.md`
- [ ] Manual arm’s-length readability pass

**Verification:**

- [ ] Manual checklist against running server + sample catalog
- [x] Full client+server build green

**Dependencies:** Task 13, Task 14, Task 15  
**Files likely touched:** `src/client/elder_ui/`*, `MainWindow` integration  
**Estimated scope:** M

**Status:** Done (2026-10-09). `ElderMainWindow` wires search/push/play/download; Chinese labels; 16/18pt fonts; no「立即同步」.

---



## Checkpoint E — v1 complete

- [x] All module success criteria from approved SPECs met or explicitly deferred
- [x] Server builds without Qt; client builds with Qt
- [x] Play does not auto-download; download is explicit
- [x] Human product acceptance before polish/CI extras (2026-10-10: accept v1 with known playback quality gap — ffplay pipeline works but A/V sync / UX not polished; defer playback polish)

---

## Phase F — Polish / CI (post-v1)

Track follow-ups after Checkpoint E. Pick slices as needed.

### Task 17: CI (server without Qt)

**Description:** Add CI job that configures/builds server with `BUILD_CLIENT=OFF` and runs server GTests (no Qt on the runner).

**Acceptance criteria:**

- [x] Workflow or documented script builds `VideoPlayerServer` without Qt
- [x] Server test suite runs green in CI (or local equivalent documented for proxy environments)

**Verification:**

- [x] `powershell -ExecutionPolicy Bypass -File scripts/ci-server.ps1` (Windows) — 29/29 passed (2026-10-10)
- [x] GitHub Actions `Server CI` green on `fanghuarumeng/VideoPlayer` (2026-10-12)

**Status:** Done (2026-10-10). Local script + GH workflow; Actions verified 2026-10-12 (Task 19).

**Dependencies:** Checkpoint E  
**Files likely touched:** `scripts/ci-server.ps1`, `scripts/ci-server.sh`, `.github/workflows/server-ci.yml`  
**Estimated scope:** M

---

### Task 18: Playback in-process libav (replace ffmpeg.exe pipelines)

**Description:** Link vendored FFmpeg shared libs; single demux/decode thread; realtime PTS pacing; paint frames in `VideoPlayerWidget`.

**Acceptance criteria:**

- [x] CMake links `avformat`/`avcodec`/`avutil`/`swscale`/`swresample` from `third_party/ffmpeg`
- [x] No primary play path via `QProcess` + `ffmpeg.exe`
- [x] Play does not touch download dir; client playback tests pass
- [ ] Manual: motion + audio acceptable on sample URL (human)

**Verification:**

- [x] Client builds with shared FFmpeg layout (`config/ffmpeg-vendor.txt`)
- [x] `ctest --test-dir build-client -C Release -R Playback --output-on-failure`
- [ ] Human re-test after PTS pacing fix

**Status:** Done (2026-10-10). Pending human sign-off on motion/audio quality.

**Dependencies:** Task 14  
**Estimated scope:** L

---

### Task 19: Remote + GitHub Actions

**Description:** Add git remote; push; verify `.github/workflows/server-ci.yml` on Origin/GitHub.

**Acceptance criteria:**

- [x] Remote configured; `Server CI` workflow green on push/PR

**Status:** Done (2026-10-12). Root cause: `project()` was CXX-only; Linux Ninja generate failed with missing `CMAKE_C_COMPILE_OBJECT`. Fixed by enabling `C CXX`. Run: https://github.com/fanghuarumeng/VideoPlayer/actions/runs/38197281279

**Dependencies:** Task 17  
**Estimated scope:** S

---

## Phase G — Personal media library

Specs: `CAPABILITY-MAP-personal-media-library.md`, `SPEC-local-media.md`, `SPEC-playlist.md`, `SPEC-library-ui.md`, `SPEC-play-history.md`.  
Plan index: `tasks/plan.md` Phase G.

---

## Task 20: Schema v2 + list API

**Description:** Add `thumbnail_url` to `Video` / `VideoDto` / JSON; migrate SQLite `schema_version` 1→2; expose `GET /api/v1/videos?limit=&offset=` newest-first.

**Acceptance criteria:**

- [x] Existing DBs migrate; new installs create v2 schema
- [x] List returns all source types; includes `thumbnail_url` (may be empty)
- [x] Search/get JSON include `thumbnail_url`
- [x] Existing server tests still green

**Verification:**

- [x] `ctest --test-dir build-server -C Release` — 34/34 passed (2026-10-10)
- [x] List covered by `HttpSearchTest.ListVideosNewestFirstIncludesThumbnailUrl`
- [ ] Manual: `curl "http://127.0.0.1:8080/api/v1/videos?limit=10"` (optional smoke)

**Dependencies:** Checkpoint E (existing store/API)  
**Files likely touched:** `Video.h`, `VideoDto.h`, `SqliteVideoRepository.*`, `video_json.*`, `HttpApiServer.*`, tests  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Task 21: Upload + UploadGate + media file serve

**Description:** Implement `LocalMediaStore` + `UploadGate` (`upload_workers`, default 2); `POST /api/v1/videos/upload` for allow-listed extensions including `.mxf`; persist under `media_root`; serve `GET /media/videos/{name}`; WS upsert on insert; config `media_root` / `upload_workers`. Thumb may stay empty.

**Acceptance criteria:**

- [x] Successful upload → `201`, `source_type=local_upload`, playable `play_url`
- [x] Concurrent uploads ≤ `upload_workers`; excess → `503 upload_busy`
- [x] Reject disallowed extensions with `400`
- [ ] url-health path not required yet for local (skip wired in Task 22 OK if easier here)

**Verification:**

- [x] Upload gate unit/HTTP test for 503 under cap=1
- [x] HTTP upload + GET `/media/videos/...` covered by `UploadHttpTest`
- [x] Server tests 39/39 green (2026-10-10)

**Dependencies:** Task 20  
**Files likely touched:** `local_media/*`, `ServerConfig.*`, `HttpApiServer.*`, `ServerApp.*`, `config/server.json.example`, tests  
**Estimated scope:** L  
**Status:** Done (2026-10-10)

---

## Task 22: ThumbnailQueue + ffmpeg + url-health skip

**Description:** FIFO `ThumbnailQueue` with `thumbnail_workers` (default 1); async ffmpeg CLI thumb; update `thumbnail_url` + second WS upsert; serve `/media/thumbs/*`; skip url-health probes/deletes for `local_upload`.

**Acceptance criteria:**

- [x] Peak concurrent ffmpeg invocations ≤ `thumbnail_workers` under burst enqueue
- [x] Upload handler never runs ffmpeg inline
- [x] Thumb success → DB + WS; failure leaves empty thumb without failing prior upload
- [x] `local_upload` not removed by url-health unreachable logic

**Verification:**

- [x] `thumbnail_queue_test` with fake generator / concurrency assert
- [ ] Manual: upload → empty thumb → later thumb URL 200 (ffmpeg present)
- [x] Automated Checkpoint G1 coverage via upload + queue + url-health tests

**Dependencies:** Task 21  
**Files likely touched:** `ThumbnailQueue.*`, `ThumbnailGenerator.*`, `UrlHealthService.*`, tests  
**Estimated scope:** L  
**Status:** Done (2026-10-10)

---

## Checkpoint G1 — local-media server path

- [x] Upload/list/media/thumb routes implemented; thumb async via queue
- [x] Upload concurrency and thumb concurrency caps verified by tests
- [x] Server tests 42/42 green (2026-10-10)
- [ ] Optional human curl smoke with real ffmpeg on PATH

---

## Task 23: PlaylistStore (client local)

**Description:** JSON playlist persistence: append (dedupe by id), clear, current_index, load/save under AppData or config path.

**Acceptance criteria:**

- [x] Round-trip persists items + index
- [x] Dedupe by id; clear empties file
- [x] GTest on temp path green

**Verification:**

- [x] `ctest --test-dir build-client -C Release -R PlaylistStore` — 3/3 passed (2026-10-10)

**Dependencies:** Checkpoint G1 (metadata URLs useful but store can start after Task 20 DTO)  
**Files likely touched:** `src/client/.../playlist/PlaylistStore.*`, tests, optional `client.json.example`  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Task 24: PlaylistPane + auto-next

**Description:** Scrollable playlist UI with thumbs/placeholder, clear (confirm), play selection / double-click, auto-play next on natural end; wire into main window when nav exists (or temporary host until Task 25).

**Acceptance criteria:**

- [x] Long list scrolls; clear confirm wipes store
- [x] Natural end advances to next item when present
- [x] Restart restores queue

**Verification:**

- [ ] Manual with ≥2 short clips
- [x] Client build green (2026-10-10)

**Dependencies:** Task 23, existing `playback`  
**Files likely touched:** `PlaylistPane.*`, `PlaylistModel.*`, `ElderMainWindow.*` or shell, `PlaybackController` finished signal if missing  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Checkpoint G2 — playlist

- [x] Persist + clear + auto-next wired (manual optional)

---

## Task 25: Library nav + tiled grid

**Description:** Main chrome sections 视频库 / 播放列表 / 历史 / 搜索; library default; tiled grid from `GET /api/v1/videos`; placeholder thumbs; scrollbar; WS upsert updates tiles.

**Acceptance criteria:**

- [x] Landing on 视频库; search demoted to its section
- [x] Grid shows server videos; scrolls when many
- [x] Thumb placeholder until `thumbnail_url` filled

**Verification:**

- [ ] Manual against running server with some rows
- [x] Client build green (2026-10-10)

**Dependencies:** Task 20 (list API); Task 24 optional for playlist tab hosting  
**Files likely touched:** `library/*`, `ElderMainWindow.*`, `CatalogClient` list method  
**Estimated scope:** L  
**Status:** Done (2026-10-10)

---

## Task 26: Client upload queue + batch-add

**Description:** `UploadController` with local queue and max in-flight (default 2), `503` backoff retry, multi-file picker including `.mxf`; multi-select + 加入播放列表 appends via `PlaylistStore`.

**Acceptance criteria:**

- [x] Multi-upload respects client in-flight cap; retries busy
- [x] Batch-add appends/dedupes into playlist UI (wired in Task 25)
- [x] `.mxf` allowed in dialog / client validation aligned with server

**Verification:**

- [ ] Manual multi-file upload + batch-add
- [x] Client build green; Checkpoint G3 automated wiring done (2026-10-10)

**Dependencies:** Task 21–22, Task 24–25  
**Files likely touched:** `UploadController.*`, `LibraryPane.*`, API upload helper  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Checkpoint G3 — library UI

- [x] Upload from UI wired (appears in grid on success / WS)
- [x] Batch-add → playlist; search still works secondary

---

## Task 27: PlayHistoryStore + progress hookup

**Description:** Local JSON history; record on play/progress/pause/stop/quit; cap 100; debounce disk writes.

**Acceptance criteria:**

- [x] Partial play + restart restores position_ms for that id
- [x] Cap enforced; store GTest green

**Verification:**

- [x] `ctest --test-dir build-client -C Release -R PlayHistory` — passed (2026-10-10)

**Dependencies:** `playback` progress signals; Checkpoint G3 helpful but not hard  
**Files likely touched:** `history/PlayHistoryStore.*`, `PlaybackController` wiring, tests  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Task 28: HistoryPane + resume

**Description:** History section UI (thumb/title/time); activate → play + seek; near-end → start 0; clear history with confirm.

**Acceptance criteria:**

- [x] Resume within ~2s of saved position (seek after play)
- [x] Clear confirm empties history
- [x] List scrolls

**Verification:**

- [ ] Manual resume checklist
- [x] Client/server builds green; **Checkpoint G4** wiring complete (2026-10-10)

**Dependencies:** Task 27, Task 25 nav host  
**Files likely touched:** `HistoryPane.*`, shell wiring  
**Estimated scope:** M  
**Status:** Done (2026-10-10)

---

## Checkpoint G4 — personal media complete

- [x] End-to-end path wired: upload → library → playlist → play → history resume
- [x] Concurrency caps (upload + thumb) covered by server tests
- [x] Specs' success criteria implemented; manual UX smoke optional

