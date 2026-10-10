# Spec: library-ui

## Objective

Primary client surface for personal media: upload local files, browse **all** videos in the server DB as a tiled thumbnail grid, multi-select, and batch-add into the local playlist. Search/push remain available but are secondary.

**User story:** User opens 视频库 → uploads or browses tiles → selects several → 加入播放列表 → switches to playlist to play.

**Depends on:** `local-media` (upload + list + thumbs), `playlist`.  
**Related:** demotes flows in `elder-ui` / current main window without deleting search.

## Tech Stack

- C++11, Qt 5 Widgets (+ Network for image fetch)
- Existing light `AppTheme` / icons where practical
- HTTP via extended `CatalogClient` (or thin `LibraryClient`) calling `GET /api/v1/videos` and `POST /api/v1/videos/upload`

## Commands

```
cmake --build build-client --config Release --target SquareDanceApp
# manual UX checklist against running SquareDanceServer with media_root + ffmpeg
```

## Project Structure

```
src/client/square_dance/client/library/
  LibraryPane.*          → grid, multi-select, batch-add, refresh
  VideoTileDelegate.*    → or custom flow layout of tiles
  UploadController.*     → QFileDialog + multipart upload; local queue + max in-flight; 503 retry
src/client/.../elder_ui/ElderMainWindow.*  → navigation shell: Library | Playlist | History | Search
src/client/.../api/      → listVideos + upload methods
tests/client/library/
  (optional) library_json_test.cpp for list payload parse
```

## UI / navigation

Main window sections (labels can be Chinese):

| Section | Role |
|---|---|
| **视频库** | Default landing; tiled grid of all DB videos |
| **播放列表** | `playlist` pane |
| **历史** | placeholder until `play-history` ships; then that pane |
| **搜索** | Existing search bar + result list (demoted, not removed) |

**视频库**

- Tiles: thumbnail + title; scrollbar when content exceeds viewport.
- Multi-select (Ctrl/Shift or checkbox affordance — pick one pattern and stick to it: **Ctrl+click toggle + “全选/清除选择”** for v1).
- Button **加入播放列表** → `PlaylistStore::append` for each selected (dedupe per playlist spec).
- Button **上传** → file picker (multi-file OK) → **bounded concurrent** uploads (default in-flight **2**, align with server `upload_workers`); queue the rest locally; on **`503 upload_busy`** retry with short backoff; refresh grid / rely on WS upsert.
- Empty state: short hint to upload or wait for ingest.
- Thumbnails may arrive **later** than the row (server thumb queue): show placeholder until WS/`reload` fills `thumbnail_url`. Include **`.mxf`** in the file dialog filter.

**Push/search:** connection status can stay in chrome; WS upsert should insert/update tiles in the library when visible (including thumb-ready updates).

## Code Style

```cpp
class LibraryPane : public QWidget {
  Q_OBJECT
 public:
  void reload();
 signals:
  void addToPlaylistRequested(const QVector<VideoDto>& videos);
};
```

- Prefer reusing `VideoDto`; do not open SQLite from UI.
- Upload progress: simple status text or determinate bar for v1.

## Testing Strategy

- Manual acceptance is the v1 gate for layout/scroll/multi-select.
- Parse tests for list JSON including `thumbnail_url`.
- Upload error paths: show message, do not crash.

## Boundaries

- **Always:** library data from HTTP; playlist mutations only via `playlist` module.
- **Ask first:** delete video from library UI; folder watch auto-upload; infinite scroll pagination UX beyond limit/offset reload.
- **Never:** elder-only “huge font only” constraint blocking this shell — keep readable professional light theme; do not auto-download on select.

## Success Criteria

- [ ] Library shows ingest + `local_upload` videos with thumbs when URLs work.
- [ ] Multi-select → batch-add appends to playlist (visible in playlist section).
- [ ] Upload local file → appears in grid after success (or WS).
- [ ] Grid scrolls with many items.
- [ ] Search section still works; not the default landing tab.

## Open Questions

- None blocking. Pagination: v1 loads first page with `limit=200`; **ask first** before cursor-based infinite scroll.
