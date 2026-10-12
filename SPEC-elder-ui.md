# Spec: elder-ui

## Objective

Elderly-friendly Windows UI on top of `client-api`: search by name (HTTP), play, explicit download, and refresh/insert rows when WebSocket pushes arrive.

**Success:** elder types a name, sees large results, plays without downloading, downloads only via button, and sees new server videos appear without manual full refresh (push).

## Tech Stack

- C++11, Qt 5 Widgets
- Depends on: `client-api`, `playback`, `download`

## Commands

```
cmake --build build --config Release --target VideoPlayerApp
# manual UX checklist
```

## Project Structure

```
src/client/elder_ui/
  ElderMainWindow.*
  SearchBar.*
  VideoResultList.*
  PlayerPane.*
  ConnectionStatusWidget.*   → WS connected/disconnected, large text OK
```

## Code Style

- Labels (zh): **搜索** / **播放** / **下载**; show sync/connection state simply (“已连接/未连接”).
- Fonts: propose **18pt** titles / **16pt** body; large row height.
- Flow: type → search (HTTP) → select → Play; Download separate.
- On `video.upserted`: insert/update list if title matches current filter or if filter empty (define: **if current query empty, prepend; if query non-empty, update only if title contains query**).
- On `video.removed`: remove from list if present.
- Optional small **重新搜索** control; do not require “立即同步” for basic use (ingest is server-side). Ops may still call HTTP `POST /api/v1/ingest/run` from a hidden/advanced menu — **ask before exposing to elders**.

## Testing Strategy

- Manual acceptance checklist is v1 gate.
- Optional presenter tests with mock CatalogClient/PushClient.

## Boundaries

- **Always:** Chinese primary UI; one main screen; play ≠ download.
- **Ask first:** exposing “立即同步” to elders; settings beyond server URL / font size.
- **Never:** direct DB; auto-download; short-video infinite feed UX.

## Success Criteria

- [ ] Name search via HTTP works end-to-end against running server.
- [ ] Play and Download behaviors match playback/download specs.
- [ ] WS upsert/remove updates the visible list per rules above.
- [ ] Manual pass: readable at arm’s length.

## Open Questions

- Expose “立即同步” to elders? Propose **no** (server timer + optional advanced menu).
- Font sizes 18/16 — confirm on approve.
