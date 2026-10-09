# Spec: server-shell

## Objective

Pure C++11 backend executable `SquareDanceServer`: CMake target, process entry, JSON config, logging, GoogleTest. **Must not link Qt.**

**Success:** server builds on Windows without Qt; loads config; smoke test passes.

## Tech Stack

- Language: C++11
- Build: CMake ≥ 3.16
- Dependencies (FetchContent or `third_party/`, pinned versions at implement time):
  - **sqlite3** (amalgamation or find_package)
  - **nlohmann/json**
  - **cpp-httplib** (HTTP server + outbound HTTP client)
  - **websocketpp** (WebSocket server; Asio transport — prefer **standalone Asio** over full Boost if feasible)
  - **GoogleTest**
- Standard library: `std::thread`, `std::mutex`, `std::atomic` for scheduling/workers
- Target OS (v1): Windows 10+

**ASSUMPTION:** cpp-httplib + websocketpp + nlohmann/json + sqlite3 are the locked backend deps (websocketpp chosen explicitly).

## Commands

```
cmake -S . -B build
cmake --build build --config Release --target SquareDanceServer
cmake --build build --config Release --target SquareDanceServerTests
ctest --test-dir build -C Release -R server --output-on-failure

build/Release/SquareDanceServer.exe --config config/server.json
```

Backend configure **must succeed without** `CMAKE_PREFIX_PATH` pointing at Qt.

## Project Structure

```
CMakeLists.txt                 → options: BUILD_SERVER, BUILD_CLIENT
cmake/
config/server.json.example
src/common/                    → Qt-free DTOs / JSON field helpers
src/server/
  main.cpp
  ServerApp.*
  ServerConfig.*
  log.*
tests/server/
  server_config_test.cpp
```

## Code Style

```cpp
namespace square_dance {
namespace server {

struct ServerConfig {
  std::string bind_host;          // "127.0.0.1"
  uint16_t http_port;             // 8080
  uint16_t ws_port;               // 8081
  std::string sqlite_path;
  int ingest_interval_ms;         // 12h
  int health_interval_ms;         // 12h
  int url_failure_threshold;      // 3
  std::vector<std::string> ingest_keywords;
  std::string authorized_catalog_path;
};

}  // namespace server
}  // namespace square_dance
```

- Use `std::string`, not `QString`, in all server and `src/common` code.
- Log to stderr + optional file.

## Testing Strategy

- GoogleTest; config parse from fixture JSON.
- CMake: assert server target does not link Qt (`Qt5::*` / `Qt6::*` forbidden on server targets).

## Boundaries

- **Always:** C++11; no Qt includes in `src/server/**` or `src/common/**`; out-of-source builds.
- **Ask first:** replacing httplib / websocketpp / nlohmann / sqlite3; forcing full Boost instead of standalone Asio.
- **Never:** link Widgets/Core Qt into server; commit secrets.

## Success Criteria

- [ ] `SquareDanceServer` builds on a machine with no Qt installed (CI job or local proof).
- [ ] Config loads from JSON example.
- [ ] Server smoke/config GTest passes via ctest.

## Open Questions

- websocketpp transport: standalone Asio vs Boost.Asio — propose **standalone Asio** unless build pain forces Boost.
