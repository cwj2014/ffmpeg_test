#CMake version
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.5)

#project information
PROJECT(FFMPEG)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -fPIC")
endif()

SET(PRONAME ffmpeg_test)

#target
ADD_EXECUTABLE(${PRONAME} src/ffmpeg_test_main.cpp
                          src/file_avframe_util.cpp
                          src/codecimpl.cpp)

SET(FFMPEG_INSTALL /home/caiyu/ffmpeg_install)
SET(X264_INSTALL /home/caiyu/x264_install)
SET(LAME_INSTALL /home/caiyu/lame_install)

INCLUDE_DIRECTORIES(${FFMPEG_INSTALL}/include)

find_library(AVFILTER_PATH avfilter ${FFMPEG_INSTALL}/lib)
find_library(AVFORMAT_PATH avformat ${FFMPEG_INSTALL}/lib)
find_library(POSTPROC_PATH postproc ${FFMPEG_INSTALL}/lib)
find_library(SWSCALE_PATH swscale ${FFMPEG_INSTALL}/lib)
find_library(SWRESAMPLE_PATH swresample ${FFMPEG_INSTALL}/lib)
find_library(AVCODEC_PATH avcodec ${FFMPEG_INSTALL}/lib)
find_library(AVUTIL_PATH avutil ${FFMPEG_INSTALL}/lib)
find_library(X264_PATH x264 ${X264_INSTALL}/lib)
find_library(LAME_PATH mp3lame ${LAME_INSTALL}/lib)


SET(LIBS ${AVFILTER_PATH}
         ${AVFORMAT_PATH}
         ${POSTPROC_PATH}
         ${SWSCALE_PATH}
         ${AVCODEC_PATH}
         ${SWRESAMPLE_PATH}
         ${AVUTIL_PATH}
         z
         lzma
         ${X264_PATH}
         ${LAME_PATH}
         pthread)

#link lib
TARGET_LINK_LIBRARIES(${PRONAME} ${LIBS})
