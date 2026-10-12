# Spec: play-history

## Objective

Record what was played and **where** it stopped, on the local client only, so the user can open history and resume.

**User story:** User watches part of a video → leaves → later opens 历史 → continues from the saved position.

**Depends on:** `playback` (position/duration signals already present or extended).  
**UI host:** `library-ui` navigation **历史** section.

## Tech Stack

- C++11, Qt 5 Widgets
- Persistence: JSON under client AppData (separate file from playlist)

## Commands

```
cmake --build build-client --config Release --target VideoPlayerApp
ctest --test-dir build-client -C Release -R play_history --output-on-failure
```

## Project Structure

```
src/client/video_player/client/history/
  PlayHistoryStore.*
  PlayHistoryModel.*
  HistoryPane.*
tests/client/history/
  play_history_store_test.cpp
```

## Behavior

- On play start: upsert history entry for `video.id` (move to most-recent).
- While playing: persist `position_ms` periodically (e.g. every 5s) and on pause/stop/seek-end / app quit.
- Entry fields: `id`, `title`, `play_url`, `thumbnail_url`, `position_ms`, `duration_ms`, `updated_at`.
- Cap list length (e.g. **100** most recent); drop oldest.
- UI: scrollable list with thumb + title + time (`mm:ss / mm:ss`); click/double-click → play that URL and **seek to `position_ms`** (use existing seek API).
- If position within last **3s** of duration (and duration known), treat as finished: next resume starts at **0** (or hide “resume” — v1: start at 0).
- Clear history control with confirm (optional but recommended; if omitted in first slice, **ask first** before shipping without it).

## Persistence format

```json
{
  "items": [
    {
      "id": 1,
      "title": "Demo",
      "play_url": "http://...",
      "thumbnail_url": "http://...",
      "position_ms": 125000,
      "duration_ms": 600000,
      "updated_at": "2026-10-10T06:00:00Z"
    }
  ]
}
```

## Code Style

```cpp
class PlayHistoryStore {
 public:
  void recordProgress(const VideoDto& video, qint64 positionMs, qint64 durationMs);
  bool load(QString* error);
  bool save(QString* error) const;
};
```

Wire from `PlaybackController` signals (`playbackProgress` / finished) — keep history module free of FFmpeg types.

## Testing Strategy

- Store upsert ordering + cap + round-trip.
- Manual: play, seek, quit, relaunch, resume near same position (±2s tolerance OK).

## Boundaries

- **Always:** local only; no server history API in v1.
- **Ask first:** sync with playlist current item; cloud backup; per-profile history.
- **Never:** block decode thread on disk I/O (debounce writes on UI/controller thread).

## Success Criteria

- [ ] After partial play + restart, history shows the item with saved position.
- [ ] Activating a history row seeks and plays from that position.
- [ ] Near-end positions resume from 0 per rule above.
- [ ] List scrolls; capped at configured max.

## Open Questions

- None blocking. Clear-history button: include in first implement slice (default **yes**).
