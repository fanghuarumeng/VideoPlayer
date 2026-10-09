# Spec: client-shell

## Objective

Windows Qt Widgets frontend executable `SquareDanceApp`: application entry, main window shell, GoogleTest harness. No direct database access.

**Success:** app window opens; can be pointed at backend base URLs via config; smoke test green.

## Tech Stack

- C++11, CMake, Qt 5 Widgets (+ Network for later client-api)
- GoogleTest
- Windows 10+

## Commands

```
cmake --build build --config Release --target SquareDanceApp
ctest --test-dir build -C Release -R client --output-on-failure
build/Release/SquareDanceApp.exe --config config/client.json
```

## Project Structure

```
config/client.json.example
src/client/
  main.cpp
  ClientApp.*
  ClientConfig.*       → http_base_url, ws_url, download_dir
  MainWindow.*         → empty shell; elder-ui fills content
tests/client/
  client_config_test.cpp
```

## Code Style

```cpp
namespace square_dance {
namespace client {

struct ClientConfig {
  QString http_base_url;  // e.g. "http://127.0.0.1:8080"
  QString ws_url;         // e.g. "ws://127.0.0.1:8081"
  QString download_dir;
};

}  // namespace client
}  // namespace square_dance
```

## Testing Strategy

- Config parse tests; MainWindow construct smoke with QApplication fixture.

## Boundaries

- **Always:** no QtSql in the client target; all catalog data via `client-api`.
- **Ask first:** bundling server launch inside the client; system tray-only mode.
- **Never:** open SQLite files; hardcode production credentials.

## Success Criteria

- [ ] `SquareDanceApp` builds and shows a window.
- [ ] Client config loads HTTP/WS endpoints.
- [ ] Client smoke GTest passes.

## Open Questions

- None blocking.
