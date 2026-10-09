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

  add_executable(SquareDanceCommonTests
    tests/common/video_json_test.cpp
  )
  target_link_libraries(SquareDanceCommonTests
    PRIVATE
      square_dance_common
      GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(SquareDanceCommonTests)
endif()

if(BUILD_CLIENT)
  message(STATUS "BUILD_CLIENT=ON (client targets added in later tasks)")
  # Client still may use square_dance_common; nlohmann pulled when server OR a
  # dedicated client JSON dep is enabled. For now require server deps OR add
  # nlohmann when only client is built.
  if(NOT BUILD_SERVER)
    include(cmake/ServerDeps.cmake)
    target_link_libraries(square_dance_common INTERFACE nlohmann_json::nlohmann_json)
  endif()
endif()
