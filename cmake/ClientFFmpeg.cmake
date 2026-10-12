# Vendored FFmpeg shared SDK under third_party/ffmpeg (see config/ffmpeg-vendor.txt).

set(VIDEO_PLAYER_FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
set(VIDEO_PLAYER_FFMPEG_INCLUDE "${VIDEO_PLAYER_FFMPEG_ROOT}/include")
set(VIDEO_PLAYER_FFMPEG_LIB_DIR "${VIDEO_PLAYER_FFMPEG_ROOT}/lib")
set(VIDEO_PLAYER_FFMPEG_BIN_DIR "${VIDEO_PLAYER_FFMPEG_ROOT}/bin")

set(_ff_hdr "${VIDEO_PLAYER_FFMPEG_INCLUDE}/libavformat/avformat.h")
if(NOT EXISTS "${_ff_hdr}")
  message(FATAL_ERROR
    "Missing FFmpeg headers at:\n  ${_ff_hdr}\n"
    "Download a Windows win64-gpl-shared build and unpack into third_party/ffmpeg/\n"
    "See config/ffmpeg-vendor.txt")
endif()

set(_ff_libs avformat avcodec avutil swscale swresample)
foreach(_name IN LISTS _ff_libs)
  if(MSVC)
    set(_lib_path "${VIDEO_PLAYER_FFMPEG_LIB_DIR}/${_name}.lib")
  else()
    set(_lib_path "${VIDEO_PLAYER_FFMPEG_LIB_DIR}/lib${_name}.a")
    if(NOT EXISTS "${_lib_path}")
      set(_lib_path "${VIDEO_PLAYER_FFMPEG_LIB_DIR}/${_name}.lib")
    endif()
  endif()
  if(NOT EXISTS "${_lib_path}")
    message(FATAL_ERROR
      "Missing FFmpeg import library:\n  ${_lib_path}\n"
      "Need a shared SDK (bin/*.dll + lib/*.lib + include/). See config/ffmpeg-vendor.txt")
  endif()
  list(APPEND VIDEO_PLAYER_FFMPEG_LIB_FILES "${_lib_path}")
endforeach()

file(GLOB VIDEO_PLAYER_FFMPEG_DLLS "${VIDEO_PLAYER_FFMPEG_BIN_DIR}/*.dll")
if(NOT VIDEO_PLAYER_FFMPEG_DLLS)
  message(FATAL_ERROR
    "No FFmpeg DLLs found in:\n  ${VIDEO_PLAYER_FFMPEG_BIN_DIR}\n"
    "See config/ffmpeg-vendor.txt")
endif()

add_library(video_player_ffmpeg INTERFACE)
target_include_directories(video_player_ffmpeg INTERFACE
  "${VIDEO_PLAYER_FFMPEG_INCLUDE}"
)
target_link_libraries(video_player_ffmpeg INTERFACE
  ${VIDEO_PLAYER_FFMPEG_LIB_FILES}
)
if(WIN32)
  target_link_libraries(video_player_ffmpeg INTERFACE ws2_32 bcrypt secur32)
endif()

function(video_player_copy_ffmpeg_dlls target_name)
  if(NOT WIN32)
    return()
  endif()
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${VIDEO_PLAYER_FFMPEG_BIN_DIR}"
      "$<TARGET_FILE_DIR:${target_name}>"
    COMMENT "copy FFmpeg shared DLLs next to ${target_name}"
  )
endfunction()
