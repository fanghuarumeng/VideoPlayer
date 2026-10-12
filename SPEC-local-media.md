# Spec: local-media

## Objective

Server accepts a local video upload, stores the file on disk, generates a thumbnail, upserts a row into `video-store`, and exposes HTTP URLs so the client can browse and play without keeping the original path.

**User story:** User picks a file in the client → server stores it → video appears in the library with a thumbnail → client plays via `play_url`.

**Depends on:** `video-store`, `server-api` (extends both).  
**Consumed by:** `playlist`, `library-ui`.

## Tech Stack

- C++11 server (no Qt), existing cpp-httplib + sqlite3 + nlohmann/json
- Thumbnail: invoke configured **`ffmpeg` CLI** (path in server config); not required to link libav into the server for v1
- Thumbnail work runs on a **bounded job queue** (see below) — never one ffmpeg process per concurrent upload unboundedly
- Disk layout under `media_root` from server config

## Commands

```
cmake --build build-server --config Release --target VideoPlayerServer
ctest --test-dir build-server -C Release -R local_media --output-on-failure
# manual:
# curl -F "file=@sample.mp4" -F "title=Demo" http://127.0.0.1:8080/api/v1/videos/upload
# curl "http://127.0.0.1:8080/api/v1/videos?limit=50&offset=0"
```

## Project Structure

```
src/server/video_player/server/local_media/
  LocalMediaStore.*          → save upload bytes, upsert row, enqueue thumb job
  ThumbnailGenerator.*       → wraps ffmpeg CLI (single job execution)
  ThumbnailQueue.*           → FIFO queue + worker pool with max concurrency
src/server/.../video_store/  → schema + Video fields extended
src/server/.../api/          → upload + list + static media routes
tests/server/local_media/
  local_media_store_test.cpp
  thumbnail_queue_test.cpp
  upload_http_test.cpp
config/server.json.example   → media_root, ffmpeg_path, thumbnail_workers
```

## Data model

Extend `Video` / `VideoDto` / JSON:

| Field | Notes |
|---|---|
| `thumbnail_url` | HTTP URL once thumb job completes; **empty while queued/in-progress or on failure**; empty also OK for legacy ingest rows |
| `source_type` | add `"local_upload"` |

SQLite: bump `schema_version` **1 → 2**; `ALTER TABLE` add `thumbnail_url TEXT NOT NULL DEFAULT ''`.

Upload row conventions:

- `external_id`: stable id derived from content hash or UUID (unique per `local_upload`)
- `title`: from form field, else original filename stem
- `play_url` / `download_url`: HTTP URLs served by this server (same URL OK for v1)
- `consecutive_failures`: `0`; url-health **skips** `local_upload` (local files are not remote probes)

Disk:

```
{media_root}/videos/{id_or_hash}.ext
{media_root}/thumbs/{id_or_hash}.jpg
```

## HTTP API

| Method | Path | Behavior |
|---|---|---|
| `POST` | `/api/v1/videos/upload` | `multipart/form-data`: required `file`; optional `title`. Subject to **upload concurrency cap** (below). On accept: saves file + DB row, **enqueues** thumb job, returns `201` + video JSON (`thumbnail_url` usually empty). Broadcast WS `video.upserted` on insert; **again** when thumb completes (non-empty `thumbnail_url`). |
| `GET` | `/api/v1/videos?limit=&offset=` | List videos newest-first (all source types). Default `limit=50`, max `200`. |
| `GET` | `/api/v1/videos/search?...` | Unchanged (existing). |
| `GET` | `/api/v1/videos/{id}` | Unchanged; include `thumbnail_url`. |
| `GET` | `/media/videos/{name}` | Serve stored video bytes (`Content-Type` by extension). |
| `GET` | `/media/thumbs/{name}` | Serve JPEG thumbnail. |

Errors: `400` bad/missing file; `413` over size limit; `503` upload slots full (`{ "error": "upload_busy" }`); `500` storage failure (JSON `{ "error": "..." }`). Thumb ffmpeg failure does **not** fail the upload; row stays with empty `thumbnail_url` (log + optional later retry **ask first**).

**Limits (v1):** allowed extensions `.mp4`, `.mkv`, `.webm`, `.avi`, `.mov`, **`.mxf`**; max body **2 GiB** (config override OK). Reject other types with `400`.

## Upload concurrency

Burst uploads must **not** unbounded-parallelize disk write + DB commit (e.g. 100 simultaneous `POST /upload` handlers chewing disk/memory).

| Rule | Requirement |
|---|---|
| Cap | At most `upload_workers` uploads in the **accept/process** critical section at once (default **2**, config max **4**) |
| Over capacity | Do **not** start another full store; respond **`503 upload_busy`** promptly (prefer rejecting before buffering a multi‑GB body when the framework allows; otherwise release slot only after handler exits) |
| Slot scope | Covers temp receive → durable write under `media_root` → DB upsert → thumb **enqueue** (not thumb ffmpeg itself) |
| Independence | Upload slots ≠ thumbnail workers; a finished upload frees its upload slot even if thumb is still queued |
| Client | Must also bound in-flight uploads (see `library-ui`); retry `503` with backoff |

## Thumbnail queue

Upload path must **not** spawn unbounded ffmpeg processes (e.g. 100 uploads ≠ 100 concurrent ffmpeg).

| Rule | Requirement |
|---|---|
| Model | FIFO job queue; each job = one video id + source path + dest thumb path |
| Concurrency | At most `thumbnail_workers` simultaneous ffmpeg processes (default **1**, config max **4**) |
| Upload API | Returns after file+DB commit; thumb generation is async |
| Completion | Update `thumbnail_url` in DB; broadcast WS `video.upserted` so library tiles refresh |
| Shutdown | Drain or cancel in-flight workers cleanly on server stop (no orphaned zombies) |
| Backpressure | Queue may grow with burst uploads; do not start extra workers beyond the cap |

## Config

Extend `config/server.json.example`:

```json
{
  "media_root": "data/media",
  "ffmpeg_path": "ffmpeg",
  "upload_workers": 2,
  "thumbnail_workers": 1
}
```

## Code Style

```cpp
struct UploadResult {
  Video video;
};

class LocalMediaStore {
 public:
  bool storeUpload(const std::string& temp_path,
                   const std::string& original_filename,
                   const std::string& title,
                   Video* out,
                   std::string* error);
};

class UploadGate {
 public:
  // RAII or tryAcquire/release; tryAcquire false → caller returns 503
  bool tryAcquire();
  void release();
};

class ThumbnailQueue {
 public:
  void start(int worker_count);
  void stop();
  void enqueue(const ThumbJob& job);  // non-blocking
};
```

- Keep SQL behind `IVideoRepository`; file I/O only in `local_media`.
- JSON field names stay snake_case, shared with client via `VideoDto`.
- Never call ffmpeg synchronously inside the HTTP upload handler.
- Never process unbounded concurrent uploads without `UploadGate`.

## Testing Strategy

- Unit: store bytes under temp `media_root`; injectable generator for thumb success/fail.
- Upload gate: with `upload_workers=1`, hold one slot and assert a second request gets `503 upload_busy` (or tryAcquire false in unit test).
- Queue: enqueue N jobs with `thumbnail_workers=1` (or 2); assert peak concurrent “ffmpeg” invocations ≤ worker cap (fake generator with latch/sleep).
- HTTP: multipart upload returns `201` with empty or pending thumb; after job runs, get-by-id / WS shows non-empty `thumbnail_url` when generator succeeds.
- Migration: open v1 DB → open with v2 code → column present.
- Extension: `.mxf` accepted by allow-list (fixture can be tiny/non-decodable for parse-only tests; decode covered when ffmpeg present).

## Boundaries

- **Always:** localhost bind; no auth; bounded upload slots + thumb workers; hard-delete of DB row should eventually allow orphan cleanup (**ask first** before auto-deleting files).
- **Ask first:** raising size limit; raising `upload_workers` or `thumbnail_workers` above 4; adding transcoding; linking libav into server instead of CLI; deleting media files on `removeById`; automatic retry of failed thumbs; server-side wait-queue instead of `503`.
- **Never:** accept upload paths as “trust client path without copying”; store secrets in media tree; require Qt on server; spawn one ffmpeg process per upload without a shared queue/cap; allow unbounded parallel upload handlers.

## Success Criteria

- [ ] Upload a local mp4 or mxf → row in SQLite with `source_type=local_upload` and non-empty `play_url`.
- [ ] Concurrent in-flight uploads on the server stay ≤ `upload_workers`; excess get `503 upload_busy`.
- [ ] Burst of many uploads keeps concurrent ffmpeg invocations ≤ `thumbnail_workers`.
- [ ] After queue drains, thumbnail file exists and `thumbnail_url` fetches `200` image bytes (when ffmpeg available); client gets a second WS upsert.
- [ ] `GET /api/v1/videos` returns the new item; first WS upsert may have empty thumb.
- [ ] Client can open `play_url` with existing in-process playback (HTTP range not required for v1 if full GET works).
- [ ] Existing search/ingest tests still green; url-health does not delete local uploads for “unreachable” HTTP.

## Open Questions

- None blocking.
