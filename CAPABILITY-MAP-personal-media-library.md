# Capability Map: personal-media-library

Approved intent: personal media client with upload → library → local playlist → local history resume.
Search/push remain available but demoted.

| Module id | Responsibility | Depends on |
|---|---|---|
| `local-media` | Server: multipart upload (incl. `.mxf`), **bounded upload slots** + **queued** thumbs (bounded ffmpeg workers), file on disk, extend video-store + HTTP/WS, serve play/thumb URLs | `video-store`, `server-api` |
| `playlist` | Client: one local persistent playlist (scroll, thumbs, clear, play from queue, auto-next) | `playback`, `local-media` |
| `library-ui` | Client: tiled library of all DB videos, multi-select batch-add to playlist, upload entry; demote search/push | `local-media`, `playlist` |
| `play-history` | Client: local history list + playback position; resume from history | `playback` |

**Build order:** `local-media` → `playlist` → `library-ui` → `play-history`

**Locked assumptions (approved):**

1. `source_type = local_upload` for uploaded rows; same library lists them with ingest sources.
2. Single global playlist; batch-add **appends** (dedupe by `id` when already present).
3. End of current item → auto-play next in playlist.
4. No auth; localhost trust (same as existing API).
5. Server serves local media files and thumbnails over HTTP (explicit exception to prior “no media proxy” for remote catalogs).
6. One Qt main window with navigation sections (library / playlist / history / search).

**Out of scope (v1):** multi-user auth; cloud sync of playlist/history; multiple named playlists; recommendations; resume queue/history on another machine.
