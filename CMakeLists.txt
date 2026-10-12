cmake_minimum_required(VERSION 3.16)
# C required for sqlite3 amalgamation and GoogleTest Threads probe on Linux.
project(VideoPlayer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/utf-8)
endif()

option(BUILD_SERVER "Build VideoPlayerServer (pure C++, no Qt)" ON)
option(BUILD_CLIENT "Build VideoPlayerApp (Qt5 Widgets)" OFF)

enable_testing()

add_library(video_player_common INTERFACE)
target_include_directories(video_player_common INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/common"
)

if(BUILD_SERVER)
  include(cmake/ServerDeps.cmake)
  target_link_libraries(video_player_common INTERFACE nlohmann_json::nlohmann_json)

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

  add_library(video_player_server_lib
    src/server/video_player/server/ServerConfig.cpp
    src/server/video_player/server/ServerApp.cpp
    src/server/video_player/server/log.cpp
    src/server/video_player/server/video_store/SqliteVideoRepository.cpp
    src/server/video_player/server/video_source/AuthorizedCatalogSource.cpp
    src/server/video_player/server/video_source/ArchiveOrgSource.cpp
    src/server/video_player/server/ingest/IngestService.cpp
    src/server/video_player/server/url_health/UrlProber.cpp
    src/server/video_player/server/url_health/UrlHealthService.cpp
    src/server/video_player/server/api/video_json.cpp
    src/server/video_player/server/api/HttpApiServer.cpp
    src/server/video_player/server/api/WsPushServer.cpp
    src/server/video_player/server/local_media/UploadGate.cpp
    src/server/video_player/server/local_media/LocalMediaStore.cpp
    src/server/video_player/server/local_media/ThumbnailGenerator.cpp
    src/server/video_player/server/local_media/ThumbnailQueue.cpp
  )
  target_include_directories(video_player_server_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/server"
  )
  target_link_libraries(video_player_server_lib
    PUBLIC
      video_player_common
      nlohmann_json::nlohmann_json
      sqlite3_amalgamation
      cpp_httplib
      websocketpp_asio
  )

  add_executable(VideoPlayerServer
    src/server/main.cpp
  )
  target_link_libraries(VideoPlayerServer PRIVATE video_player_server_lib)

  get_target_property(_server_libs VideoPlayerServer LINK_LIBRARIES)
  if(_server_libs MATCHES "Qt5::|Qt6::")
    message(FATAL_ERROR "VideoPlayerServer must not link Qt")
  endif()

  add_executable(VideoPlayerCommonTests
    tests/common/video_json_test.cpp
  )
  target_link_libraries(VideoPlayerCommonTests
    PRIVATE
      video_player_common
      GTest::gtest_main
  )

  add_executable(VideoPlayerServerTests
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
  target_link_libraries(VideoPlayerServerTests
    PRIVATE
      video_player_server_lib
      GTest::gtest_main
  )
  file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" _video_player_source_dir)
  target_compile_definitions(VideoPlayerServerTests PRIVATE
    "VIDEO_PLAYER_SOURCE_DIR=\"${_video_player_source_dir}\""
  )

  include(GoogleTest)
  gtest_discover_tests(VideoPlayerCommonTests)
  gtest_discover_tests(VideoPlayerServerTests)
endif()

if(BUILD_CLIENT)
  if(NOT BUILD_SERVER)
    include(cmake/ServerDeps.cmake)
    target_link_libraries(video_player_common INTERFACE nlohmann_json::nlohmann_json)
  endif()

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  find_package(Qt5 REQUIRED COMPONENTS
    Widgets Network WebSockets Multimedia Test Svg
    Qml Quick QuickControls2)
  include(cmake/ClientFFmpeg.cmake)

  set(VIDEO_PLAYER_CLIENT_RESOURCES
    src/client/resources/app.qrc
  )

  add_library(video_player_client_lib
    src/client/video_player/client/ClientConfig.cpp
    src/client/video_player/client/ClientApp.cpp
    src/client/video_player/client/MainWindow.cpp
    src/client/video_player/client/api/catalog_json.cpp
    src/client/video_player/client/api/CatalogClient.cpp
    src/client/video_player/client/api/CatalogModel.cpp
    src/client/video_player/client/api/push_json.cpp
    src/client/video_player/client/api/PushClient.cpp
    src/client/video_player/client/playback/FfmpegPlaybackEngine.cpp
    src/client/video_player/client/playback/PlaybackController.cpp
    src/client/video_player/client/playback/VideoPlayerWidget.cpp
    src/client/video_player/client/download/DownloadService.cpp
    src/client/video_player/client/elder_ui/AppIcons.cpp
    src/client/video_player/client/elder_ui/VideoListDelegate.cpp
    src/client/video_player/client/elder_ui/ConnectionStatusWidget.cpp
    src/client/video_player/client/elder_ui/SearchBar.cpp
    src/client/video_player/client/elder_ui/VideoResultList.cpp
    src/client/video_player/client/elder_ui/BufferedSeekSlider.cpp
    src/client/video_player/client/elder_ui/PlayerPane.cpp
    src/client/video_player/client/elder_ui/ElderMainWindow.cpp
    src/client/video_player/client/playlist/PlaylistStore.cpp
    src/client/video_player/client/playlist/PlaylistModel.cpp
    src/client/video_player/client/playlist/PlaylistPane.cpp
    src/client/video_player/client/library/LibraryPane.cpp
    src/client/video_player/client/library/UploadController.cpp
    src/client/video_player/client/history/PlayHistoryStore.cpp
    src/client/video_player/client/history/HistoryPane.cpp
    src/client/video_player/client/history/HistoryModel.cpp
    src/client/video_player/client/qml/VideoFrameItem.cpp
    src/client/video_player/client/qml/PlayerUiBridge.cpp
    src/client/video_player/client/qml/SecondaryPaneHost.cpp
    ${VIDEO_PLAYER_CLIENT_RESOURCES}
  )
  target_include_directories(video_player_client_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/client"
  )
  target_link_libraries(video_player_client_lib
    PUBLIC
      video_player_common
      nlohmann_json::nlohmann_json
      Qt5::Widgets
      Qt5::Network
      Qt5::WebSockets
      Qt5::Multimedia
      Qt5::Svg
      Qt5::Qml
      Qt5::Quick
      Qt5::QuickControls2
      video_player_ffmpeg
  )

  add_executable(VideoPlayerApp
    src/client/main.cpp
  )
  target_link_libraries(VideoPlayerApp PRIVATE video_player_client_lib)

  get_target_property(_client_libs video_player_client_lib LINK_LIBRARIES)
  if(_client_libs MATCHES "Qt5::Sql|Qt6::Sql")
    message(FATAL_ERROR "Client must not link QtSql")
  endif()
  get_target_property(_app_libs VideoPlayerApp LINK_LIBRARIES)
  if(_app_libs MATCHES "Qt5::Sql|Qt6::Sql")
    message(FATAL_ERROR "VideoPlayerApp must not link QtSql")
  endif()

  add_executable(VideoPlayerClientTests
    tests/client/client_config_test.cpp
    tests/client/api/catalog_json_test.cpp
    tests/client/api/catalog_model_test.cpp
    tests/client/api/catalog_client_test.cpp
    tests/client/api/push_json_test.cpp
    tests/client/api/push_model_test.cpp
    tests/client/api/push_client_test.cpp
    tests/client/playback/playback_no_download_test.cpp
    tests/client/playback/playback_rapid_replay_test.cpp
    tests/client/download/download_service_test.cpp
    tests/client/playlist/playlist_store_test.cpp
    tests/client/history/play_history_store_test.cpp
  )
  target_link_libraries(VideoPlayerClientTests
    PRIVATE
      video_player_client_lib
      GTest::gtest
      Qt5::Test
  )
  file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" _video_player_client_source_dir)
  target_compile_definitions(VideoPlayerClientTests PRIVATE
    "VIDEO_PLAYER_SOURCE_DIR=\"${_video_player_client_source_dir}\""
  )
  # Deploy Qt + FFmpeg shared DLLs next to exes.
  if(WIN32)
    get_filename_component(_qt5_prefix "${Qt5_DIR}/../../.." ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
      HINTS "${_qt5_prefix}/bin"
      REQUIRED
    )
    foreach(_deploy_target VideoPlayerApp VideoPlayerClientTests)
      add_custom_command(TARGET ${_deploy_target} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
          --no-translations
          --no-compiler-runtime
          --multimedia
          "$<TARGET_FILE:${_deploy_target}>"
        COMMENT "windeployqt ${_deploy_target}"
      )
      video_player_copy_ffmpeg_dlls(${_deploy_target})
    endforeach()
  endif()

  include(GoogleTest)
  gtest_discover_tests(VideoPlayerClientTests)
endif()
