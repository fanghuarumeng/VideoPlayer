# Spec: playlist

## Objective

One client-local playback queue: show title + thumbnail, scroll when long, clear all, play from selection, auto-advance to next when the current item finishes.

**User story:** User builds a queue → plays → finishes one → next starts → can wipe the queue.

**Depends on:** `playback`; uses video metadata/`thumbnail_url` from `local-media` (or existing catalog DTOs).  
**Consumed by:** `library-ui` (batch-add).

## Tech Stack

- C++11, Qt 5 Widgets
- Persistence: JSON file under client data dir (path from config or `QStandardPaths::AppDataLocation`), **not** SQLite, **not** server

## Commands

```
cmake --build build-client --config Release --target VideoPlayerApp
ctest --test-dir build-client -C Release -R playlist --output-on-failure
```

## Project Structure

```
src/client/video_player/client/playlist/
  PlaylistStore.*       → load/save JSON; append/clear/remove
  PlaylistModel.*       → QAbstractListModel (id, title, play_url, thumbnail_url)
  PlaylistPane.*        → list UI + clear + play current
tests/client/playlist/
  playlist_store_test.cpp
config/client.json.example → optional playlist_path
```

## Behavior

- **Single** global playlist.
- **Append** videos by `id`; if `id` already present, skip (dedupe).
- Persist after every structural change (add / remove / clear).
- UI: scrollable list; each row shows thumbnail (async load from `thumbnail_url`, placeholder if empty/fail) + title.
- **Clear playlist:** one control; confirm dialog (“清空播放列表？”) then wipe store + model.
- **Play:** selecting + Play (or double-click) starts `PlaybackController` with that item’s `play_url`; set current index.
- **Auto-next:** on playback finished (natural end), if next index exists, play it; else stop.
- Restart app → restore last saved queue and current index (index best-effort; if out of range, clamp to 0 or none).

## Persistence format

```json
{
  "current_index": 0,
  "items": [
    {
      "id": 1,
      "title": "Demo",
      "play_url": "http://127.0.0.1:8080/media/videos/...",
      "thumbnail_url": "http://127.0.0.1:8080/media/thumbs/..."
    }
  ]
}
```

Snapshot URLs/titles at add time so the list still renders if the server is briefly down (playback may still fail until server is up).

## Code Style

```cpp
class PlaylistStore {
 public:
  bool load(QString* error);
  bool save(QString* error) const;
  bool append(const VideoDto& video);  // dedupe by id
  void clear();
  QVector<PlaylistItem> items() const;
  int currentIndex() const;
  void setCurrentIndex(int index);
};
```

## Testing Strategy

- Store round-trip on temp JSON path; dedupe; clear.
- Optional model unit test without UI.
- Manual: add many items → scrollbar; clear confirm; auto-next across two short clips.

## Boundaries

- **Always:** local file only; no server playlist API in v1.
- **Ask first:** reorder drag-drop; remove-one UI; multiple playlists.
- **Never:** write playlist into server DB; block UI thread on thumbnail download (use async).

## Success Criteria

- [ ] Append + restart app → same order and titles/thumbs metadata restored.
- [ ] Clear empties UI and file.
- [ ] Long list scrolls.
- [ ] Natural end of item N starts item N+1 when present.
- [ ] Dedupe: adding the same `id` twice does not create two rows.

## Open Questions

- None blocking.
