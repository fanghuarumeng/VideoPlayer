# Spec: ingest

## Objective

Backend scheduler (C++11 threads): every **12 hours** and on `runOnce()`, search all sources and upsert. Notify listeners (callback/observer) so `server-api` can WebSocket-push.

## Tech Stack

- C++11 `std::thread` / condition_variable timer loop
- No Qt / no QTimer

## Commands

```
ctest --test-dir build -C Release -R ingest --output-on-failure
```

## Project Structure

```
src/server/ingest/
  IngestService.*
  IngestConfig.h
tests/server/ingest/
  ingest_service_test.cpp
```

## Code Style

```cpp
class IngestService {
 public:
  using UpsertedHandler = std::function<void(const std::vector<Video>&)>;
  void set_upserted_handler(UpsertedHandler handler);
  void start();
  void stop();
  void runOnce();
};
```

- Default interval 12h; isolate source failures; do not block the HTTP accept loop (own worker thread).

## Testing Strategy

- Fake sources + temp repo; dedupe; handler invoked on insert.

## Boundaries

- **Always:** 12h default; no media download; notify API layer via handler (not WS directly).
- **Ask first:** OS task scheduler instead of in-process thread.
- **Never:** Qt; delete rows.

## Success Criteria

- [ ] Timer + runOnce work.
- [ ] Dedupe tests green.
- [ ] Handler hook ready for server-api.

## Open Questions

- None.
