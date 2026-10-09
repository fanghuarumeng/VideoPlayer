# Spec: download

## Objective

Frontend downloads `download_url` to local disk **only** when the user clicks Download.

**Success:** explicit click creates a file under `ClientConfig::download_dir`; play path never invokes this module.

## Tech Stack

- C++11, Qt 5 Network
- Depends on: `client-api` DTO, `client-shell` paths

## Commands

```
ctest --test-dir build -C Release -R download --output-on-failure
```

## Project Structure

```
src/client/download/
  DownloadService.*
tests/client/download/
  download_service_test.cpp
```

## Code Style

```cpp
class DownloadService : public QObject {
  Q_OBJECT
 public:
  void startDownload(const Video& video);
  void cancel(qint64 video_id);
 signals:
  void progress(qint64 video_id, qint64 received, qint64 total);
  void finished(qint64 video_id, const QString& localPath);
  void errorOccurred(qint64 video_id, const QString& message);
};
```

- v1: simple single/sequential downloads; no complex queue.
- Cancel deletes partial file.
- Local path stays on client only (not written back to server DB).

## Testing Strategy

- Tiny HTTP fixture/file URL → assert output file exists.

## Boundaries

- **Always:** user-triggered only; write under download_dir.
- **Ask first:** resume; parallel multi-download; telling server about local paths.
- **Never:** hook from `PlaybackController::play`.

## Success Criteria

- [ ] Click download saves file.
- [ ] Progress/completion signals work.
- [ ] Playback tests remain free of download coupling.

## Open Questions

- None (server `local_path` rejected — client-only).
