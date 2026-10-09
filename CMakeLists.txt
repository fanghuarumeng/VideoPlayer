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

  add_library(square_dance_server_lib
    src/server/square_dance/server/ServerConfig.cpp
    src/server/square_dance/server/ServerApp.cpp
    src/server/square_dance/server/log.cpp
  )
  target_include_directories(square_dance_server_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/server"
  )
  target_link_libraries(square_dance_server_lib
    PUBLIC
      square_dance_common
      nlohmann_json::nlohmann_json
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
