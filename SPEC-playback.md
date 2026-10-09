# Spec: playback

## Objective

Frontend playback of a selected video’s `play_url`. Must **not** download or write media files.

**Success:** play/pause/stop works for reachable http(s) URLs; download directory unchanged by play-only use.

## Tech Stack

- C++11, Qt 5 Multimedia + Widgets
- Depends on: `client-api` (Video DTO), `client-shell`

## Commands

```
ctest --test-dir build -C Release -R playback --output-on-failure
```

## Project Structure

```
src/client/playback/
  PlaybackController.*
  VideoPlayerWidget.*
tests/client/playback/
  playback_no_download_test.cpp
```

## Code Style

```cpp
class PlaybackController : public QObject {
  Q_OBJECT
 public:
  void play(const Video& video);
  void pause();
  void stop();
 signals:
  void errorOccurred(const QString& message);
};
```

- Uses `play_url` only; must not call `DownloadService`.

## Testing Strategy

- Assert download dir file count unchanged across play session (with mock/local URL when possible).

## Boundaries

- **Always:** play ≠ download; errors via signals.
- **Ask first:** external player (mpv/VLC); server-side streaming proxy.
- **Never:** auto-download on play; save full stream to disk as silent cache.

## Success Criteria

- [ ] Play/pause/stop for selected video.
- [ ] No download side effects from play.
- [ ] Bad URL surfaces error without crash.

## Open Questions

- None.
