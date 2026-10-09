# Spec: url-health

## Objective

Pure C++ URL probing. After **3** consecutive failures, hard-delete and notify `server-api` for WS `video.removed`.

## Tech Stack

- C++11, cpp-httplib client (HEAD / short GET)
- Worker thread for schedule (12h default)
- No Qt

## Commands

```
ctest --test-dir build -C Release -R url_health --output-on-failure
```

## Project Structure

```
src/server/url_health/
  UrlHealthService.*
  UrlProber.*
tests/server/url_health/
  url_health_service_test.cpp
```

## Code Style

```cpp
class UrlHealthService {
 public:
  static const int kFailureThreshold = 3;
  using RemovedHandler = std::function<void(const std::vector<int64_t>&)>;
  void set_removed_handler(RemovedHandler handler);
  void start();
  void stop();
  void runOnce();
};
```

Probe rules unchanged: HEAD then fallback; both URLs if different; success resets counter.

## Testing Strategy

- Inject mock prober; threshold-3 delete + handler.

## Boundaries

- **Always:** threshold 3; hard delete; no Qt.
- **Ask first:** change interval/threshold.
- **Never:** full-file download as probe.

## Success Criteria

- [ ] Unit tests for threshold + handler.
- [ ] Does not stall HTTP server thread.

## Open Questions

- None.
