# Prefer vendored third_party (FetchContent to GitHub often blocked by corporate proxy).

set(SQUARE_DANCE_NLOHMANN_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/nlohmann")
if(NOT EXISTS "${SQUARE_DANCE_NLOHMANN_INCLUDE}/json.hpp")
  message(FATAL_ERROR
    "Missing third_party/nlohmann/json.hpp (nlohmann/json single-header v3.11.3).")
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
  add_library(nlohmann_json INTERFACE)
  add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
  target_include_directories(nlohmann_json INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party"
  )
endif()

set(SQUARE_DANCE_GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/googletest")
if(NOT EXISTS "${SQUARE_DANCE_GTEST_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR
    "Missing third_party/googletest (expected GoogleTest v1.14.0 tree).")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("${SQUARE_DANCE_GTEST_ROOT}" "${CMAKE_BINARY_DIR}/_deps/googletest" EXCLUDE_FROM_ALL)

# Vendored separately: third_party/sqlite (amalgamation).
# Later tasks may vendor: cpp-httplib, websocketpp under third_party/.
