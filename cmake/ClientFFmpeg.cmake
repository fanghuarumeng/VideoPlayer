# Vendored FFmpeg shared SDK under third_party/ffmpeg (see config/ffmpeg-vendor.txt).

set(SQUARE_DANCE_FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
set(SQUARE_DANCE_FFMPEG_INCLUDE "${SQUARE_DANCE_FFMPEG_ROOT}/include")
set(SQUARE_DANCE_FFMPEG_LIB_DIR "${SQUARE_DANCE_FFMPEG_ROOT}/lib")
set(SQUARE_DANCE_FFMPEG_BIN_DIR "${SQUARE_DANCE_FFMPEG_ROOT}/bin")

set(_ff_hdr "${SQUARE_DANCE_FFMPEG_INCLUDE}/libavformat/avformat.h")
if(NOT EXISTS "${_ff_hdr}")
  message(FATAL_ERROR
    "Missing FFmpeg headers at:\n  ${_ff_hdr}\n"
    "Download a Windows win64-gpl-shared build and unpack into third_party/ffmpeg/\n"
    "See config/ffmpeg-vendor.txt")
endif()

set(_ff_libs avformat avcodec avutil swscale swresample)
foreach(_name IN LISTS _ff_libs)
  if(MSVC)
    set(_lib_path "${SQUARE_DANCE_FFMPEG_LIB_DIR}/${_name}.lib")
  else()
    set(_lib_path "${SQUARE_DANCE_FFMPEG_LIB_DIR}/lib${_name}.a")
    if(NOT EXISTS "${_lib_path}")
      set(_lib_path "${SQUARE_DANCE_FFMPEG_LIB_DIR}/${_name}.lib")
    endif()
  endif()
  if(NOT EXISTS "${_lib_path}")
    message(FATAL_ERROR
      "Missing FFmpeg import library:\n  ${_lib_path}\n"
      "Need a shared SDK (bin/*.dll + lib/*.lib + include/). See config/ffmpeg-vendor.txt")
  endif()
  list(APPEND SQUARE_DANCE_FFMPEG_LIB_FILES "${_lib_path}")
endforeach()

file(GLOB SQUARE_DANCE_FFMPEG_DLLS "${SQUARE_DANCE_FFMPEG_BIN_DIR}/*.dll")
if(NOT SQUARE_DANCE_FFMPEG_DLLS)
  message(FATAL_ERROR
    "No FFmpeg DLLs found in:\n  ${SQUARE_DANCE_FFMPEG_BIN_DIR}\n"
    "See config/ffmpeg-vendor.txt")
endif()

add_library(square_dance_ffmpeg INTERFACE)
target_include_directories(square_dance_ffmpeg INTERFACE
  "${SQUARE_DANCE_FFMPEG_INCLUDE}"
)
target_link_libraries(square_dance_ffmpeg INTERFACE
  ${SQUARE_DANCE_FFMPEG_LIB_FILES}
)
if(WIN32)
  target_link_libraries(square_dance_ffmpeg INTERFACE ws2_32 bcrypt secur32)
endif()

function(square_dance_copy_ffmpeg_dlls target_name)
  if(NOT WIN32)
    return()
  endif()
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${SQUARE_DANCE_FFMPEG_BIN_DIR}"
      "$<TARGET_FILE_DIR:${target_name}>"
    COMMENT "copy FFmpeg shared DLLs next to ${target_name}"
  )
endfunction()
