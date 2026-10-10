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
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/websocketpp/websocketpp/server.hpp")
    message(FATAL_ERROR "Missing third_party/websocketpp")
  endif()
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/asio/asio.hpp")
    message(FATAL_ERROR "Missing third_party/asio (standalone include tree)")
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

  add_library(websocketpp_asio INTERFACE)
  target_include_directories(websocketpp_asio INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party/websocketpp"
    "${CMAKE_SOURCE_DIR}/third_party/asio"
  )
  target_compile_definitions(websocketpp_asio INTERFACE
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STL_
  )
  if(WIN32)
    target_compile_definitions(websocketpp_asio INTERFACE _WIN32_WINNT=0x0601)
    target_link_libraries(websocketpp_asio INTERFACE ws2_32)
  endif()

  add_library(square_dance_server_lib
    src/server/square_dance/server/ServerConfig.cpp
    src/server/square_dance/server/ServerApp.cpp
    src/server/square_dance/server/log.cpp
    src/server/square_dance/server/video_store/SqliteVideoRepository.cpp
    src/server/square_dance/server/video_source/AuthorizedCatalogSource.cpp
    src/server/square_dance/server/video_source/ArchiveOrgSource.cpp
    src/server/square_dance/server/ingest/IngestService.cpp
    src/server/square_dance/server/url_health/UrlProber.cpp
    src/server/square_dance/server/url_health/UrlHealthService.cpp
    src/server/square_dance/server/api/video_json.cpp
    src/server/square_dance/server/api/HttpApiServer.cpp
    src/server/square_dance/server/api/WsPushServer.cpp
    src/server/square_dance/server/local_media/UploadGate.cpp
    src/server/square_dance/server/local_media/LocalMediaStore.cpp
    src/server/square_dance/server/local_media/ThumbnailGenerator.cpp
    src/server/square_dance/server/local_media/ThumbnailQueue.cpp
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
      websocketpp_asio
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
    tests/server/server_app_test.cpp
    tests/server/video_store/sqlite_repository_test.cpp
    tests/server/video_source/authorized_catalog_test.cpp
    tests/server/video_source/archive_org_parse_test.cpp
    tests/server/ingest/ingest_service_test.cpp
    tests/server/url_health/url_health_service_test.cpp
    tests/server/api/video_json_test.cpp
    tests/server/api/http_search_test.cpp
    tests/server/api/ws_push_test.cpp
    tests/server/local_media/upload_gate_test.cpp
    tests/server/local_media/upload_http_test.cpp
    tests/server/local_media/thumbnail_queue_test.cpp
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
  if(NOT BUILD_SERVER)
    include(cmake/ServerDeps.cmake)
    target_link_libraries(square_dance_common INTERFACE nlohmann_json::nlohmann_json)
  endif()

  set(CMAKE_AUTOMOC ON)
  find_package(Qt5 REQUIRED COMPONENTS Widgets Network WebSockets Multimedia Test Svg)
  include(cmake/ClientFFmpeg.cmake)

  set(SQUARE_DANCE_CLIENT_RESOURCES
    src/client/resources/app.qrc
  )

  add_library(square_dance_client_lib
    src/client/square_dance/client/ClientConfig.cpp
    src/client/square_dance/client/ClientApp.cpp
    src/client/square_dance/client/MainWindow.cpp
    src/client/square_dance/client/api/catalog_json.cpp
    src/client/square_dance/client/api/CatalogClient.cpp
    src/client/square_dance/client/api/CatalogModel.cpp
    src/client/square_dance/client/api/push_json.cpp
    src/client/square_dance/client/api/PushClient.cpp
    src/client/square_dance/client/playback/FfmpegPlaybackEngine.cpp
    src/client/square_dance/client/playback/PlaybackController.cpp
    src/client/square_dance/client/playback/VideoPlayerWidget.cpp
    src/client/square_dance/client/download/DownloadService.cpp
    src/client/square_dance/client/elder_ui/AppIcons.cpp
    src/client/square_dance/client/elder_ui/VideoListDelegate.cpp
    src/client/square_dance/client/elder_ui/ConnectionStatusWidget.cpp
    src/client/square_dance/client/elder_ui/SearchBar.cpp
    src/client/square_dance/client/elder_ui/VideoResultList.cpp
    src/client/square_dance/client/elder_ui/PlayerPane.cpp
    src/client/square_dance/client/elder_ui/ElderMainWindow.cpp
    src/client/square_dance/client/playlist/PlaylistStore.cpp
    src/client/square_dance/client/playlist/PlaylistModel.cpp
    src/client/square_dance/client/playlist/PlaylistPane.cpp
    src/client/square_dance/client/library/LibraryPane.cpp
    src/client/square_dance/client/library/UploadController.cpp
    src/client/square_dance/client/history/PlayHistoryStore.cpp
    ${SQUARE_DANCE_CLIENT_RESOURCES}
  )
  target_include_directories(square_dance_client_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client"
  )
  target_link_libraries(square_dance_client_lib
    PUBLIC
      square_dance_common
      nlohmann_json::nlohmann_json
      Qt5::Widgets
      Qt5::Network
      Qt5::WebSockets
      Qt5::Multimedia
      Qt5::Svg
      square_dance_ffmpeg
  )

  add_executable(SquareDanceApp
    src/client/main.cpp
  )
  target_link_libraries(SquareDanceApp PRIVATE square_dance_client_lib)

  get_target_property(_client_libs square_dance_client_lib LINK_LIBRARIES)
  if(_client_libs MATCHES "Qt5::Sql|Qt6::Sql")
    message(FATAL_ERROR "Client must not link QtSql")
  endif()
  get_target_property(_app_libs SquareDanceApp LINK_LIBRARIES)
  if(_app_libs MATCHES "Qt5::Sql|Qt6::Sql")
    message(FATAL_ERROR "SquareDanceApp must not link QtSql")
  endif()

  add_executable(SquareDanceClientTests
    tests/client/client_config_test.cpp
    tests/client/api/catalog_json_test.cpp
    tests/client/api/catalog_model_test.cpp
    tests/client/api/catalog_client_test.cpp
    tests/client/api/push_json_test.cpp
    tests/client/api/push_model_test.cpp
    tests/client/api/push_client_test.cpp
    tests/client/playback/playback_no_download_test.cpp
    tests/client/download/download_service_test.cpp
    tests/client/playlist/playlist_store_test.cpp
    tests/client/history/play_history_store_test.cpp
  )
  target_link_libraries(SquareDanceClientTests
    PRIVATE
      square_dance_client_lib
      GTest::gtest
      Qt5::Test
  )
  file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" _square_dance_client_source_dir)
  target_compile_definitions(SquareDanceClientTests PRIVATE
    "SQUARE_DANCE_SOURCE_DIR=\"${_square_dance_client_source_dir}\""
  )
  # Deploy Qt + FFmpeg shared DLLs next to exes.
  if(WIN32)
    get_filename_component(_qt5_prefix "${Qt5_DIR}/../../.." ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
      HINTS "${_qt5_prefix}/bin"
      REQUIRED
    )
    foreach(_deploy_target SquareDanceApp SquareDanceClientTests)
      add_custom_command(TARGET ${_deploy_target} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
          --no-translations
          --no-compiler-runtime
          --multimedia
          "$<TARGET_FILE:${_deploy_target}>"
        COMMENT "windeployqt ${_deploy_target}"
      )
      square_dance_copy_ffmpeg_dlls(${_deploy_target})
    endforeach()
  endif()

  include(GoogleTest)
  gtest_discover_tests(SquareDanceClientTests)
endif()
