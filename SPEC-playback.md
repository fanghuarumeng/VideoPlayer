# Spec: playback

## Objective

Frontend playback of a selected video’s `play_url`. Must **not** download or write media files.

**Success:** play/pause/stop works for reachable http(s) / local file URLs; download directory unchanged by play-only use.

## Tech Stack

- C++11, Qt 5 Widgets + Qt Multimedia (`QAudioOutput` only)
- **In-process FFmpeg shared libraries** (`libavformat` / `libavcodec` / `libavutil` / `libswscale` / `libswresample`) vendored under `third_party/ffmpeg` — see `config/ffmpeg-vendor.txt`
- Depends on: `client-api` (Video DTO), `client-shell`

## Commands

```
ctest --test-dir build-client -C Release -R playback --output-on-failure
```

## Project Structure

```
src/client/playback/
  FfmpegPlaybackEngine.*   # demux/decode worker thread
  PlaybackController.*     # clock, audio out, UI wiring
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
- Single demux → decode; audio clock drives video present (not dual `ffmpeg.exe` processes).

## Testing Strategy

- Assert download dir file count unchanged across play session (with mock/local URL when possible).

## Boundaries

- **Always:** play ≠ download; errors via signals.
- **Ask first:** embedding libmpv/libVLC instead of libav; server-side streaming proxy.
- **Never:** auto-download on play; save full stream to disk as silent cache; shell out to `ffmpeg.exe`/`ffplay.exe` as the primary play path; open the system default player as a silent fallback.

## Success Criteria

- [ ] Play/pause/stop for selected video.
- [ ] No download side effects from play.
- [ ] Bad URL surfaces error without crash.

## Open Questions

- None.
