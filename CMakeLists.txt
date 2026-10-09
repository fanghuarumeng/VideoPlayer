cmake_minimum_required(VERSION 3.16)
project(SquareDance VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8)
endif()

option(BUILD_SERVER "Build SquareDanceServer (pure C++, no Qt)" ON)
option(BUILD_CLIENT "Build SquareDanceApp (Qt5 Widgets)" OFF)

enable_testing()

add_library(square_dance_common INTERFACE)
target_include_directories(square_dance_common INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/common"
)

if(BUILD_SERVER)
  include(cmake/ServerDeps.cmake)
  target_link_libraries(square_dance_common INTERFACE nlohmann_json::nlohmann_json)

  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c")
    message(FATAL_ERROR "Missing third_party/sqlite/sqlite3.c amalgamation")
  endif()
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/cpp-httplib/httplib.h")
    message(FATAL_ERROR "Missing third_party/cpp-httplib/httplib.h")
  endif()
  add_library(sqlite3_amalgamation STATIC
    third_party/sqlite/sqlite3.c
  )
  target_include_directories(sqlite3_amalgamation PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/sqlite"
  )
  if(MSVC)
    target_compile_definitions(sqlite3_amalgamation PRIVATE SQLITE_API=)
  endif()

  add_library(cpp_httplib INTERFACE)
  target_include_directories(cpp_httplib INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party/cpp-httplib"
  )
  if(WIN32)
    target_link_libraries(cpp_httplib INTERFACE ws2_32)
  endif()

  add_library(square_dance_server_lib
    src/server/square_dance/server/ServerConfig.cpp
    src/server/square_dance/server/ServerApp.cpp
    src/server/square_dance/server/log.cpp
    src/server/square_dance/server/video_store/SqliteVideoRepository.cpp
    src/server/square_dance/server/video_source/AuthorizedCatalogSource.cpp
    src/server/square_dance/server/ingest/IngestService.cpp
    src/server/square_dance/server/api/video_json.cpp
    src/server/square_dance/server/api/HttpApiServer.cpp
  )
  target_include_directories(square_dance_server_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/server"
  )
  target_link_libraries(square_dance_server_lib
    PUBLIC
      square_dance_common
      nlohmann_json::nlohmann_json
      sqlite3_amalgamation
      cpp_httplib
  )

  add_executable(SquareDanceServer
    src/server/main.cpp
  )
  target_link_libraries(SquareDanceServer PRIVATE square_dance_server_lib)

  get_target_property(_server_libs SquareDanceServer LINK_LIBRARIES)
  if(_server_libs MATCHES "Qt5::|Qt6::")
    message(FATAL_ERROR "SquareDanceServer must not link Qt")
  endif()

  add_executable(SquareDanceCommonTests
    tests/common/video_json_test.cpp
  )
  target_link_libraries(SquareDanceCommonTests
    PRIVATE
      square_dance_common
      GTest::gtest_main
  )

  add_executable(SquareDanceServerTests
    tests/server/server_config_test.cpp
    tests/server/video_store/sqlite_repository_test.cpp
    tests/server/video_source/authorized_catalog_test.cpp
    tests/server/ingest/ingest_service_test.cpp
    tests/server/api/video_json_test.cpp
    tests/server/api/http_search_test.cpp
  )
  target_link_libraries(SquareDanceServerTests
    PRIVATE
      square_dance_server_lib
      GTest::gtest_main
  )
  file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" _square_dance_source_dir)
  target_compile_definitions(SquareDanceServerTests PRIVATE
    "SQUARE_DANCE_SOURCE_DIR=\"${_square_dance_source_dir}\""
  )

  include(GoogleTest)
  gtest_discover_tests(SquareDanceCommonTests)
  gtest_discover_tests(SquareDanceServerTests)
endif()

if(BUILD_CLIENT)
  message(STATUS "BUILD_CLIENT=ON (client targets added in later tasks)")
  if(NOT BUILD_SERVER)
    include(cmake/ServerDeps.cmake)
    target_link_libraries(square_dance_common INTERFACE nlohmann_json::nlohmann_json)
  endif()
endif()
