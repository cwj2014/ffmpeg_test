#CMake version
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.5)

#project information
PROJECT(FFMPEG)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -fPIC")
endif()

SET(PRONAME ffmpeg_test)
SET(LIBTCIFNAME truecut_tcif)

#target
ADD_EXECUTABLE(${PRONAME} src/ffmpeg_test_main.cpp
                          src/avframe_util.cpp
                          src/codecimpl.cpp)

 ADD_LIBRARY(${LIBTCIFNAME} src/truecut_tcif.cpp)           

SET(FFMPEG_INSTALL /home/caiyu/install)
SET(X264_INSTALL /home/caiyu/install)
SET(X265_INSTALL /home/caiyu/install)
SET(LAME_INSTALL /home/caiyu/install)
SET(HEIF_ROOT /home/caiyu/source/heif)
SET(EXIV2_INSTALL /home/caiyu/install)

INCLUDE_DIRECTORIES(${FFMPEG_INSTALL}/include 
                    ${HEIF_ROOT}/srcs/api/common 
                    ${HEIF_ROOT}/srcs/api/reader 
                    ${HEIF_ROOT}/srcs/api/writer
                    ${HEIF_ROOT}/srcs/api-cpp
                    ${EXIV2_INSTALL}/include
                    )

find_library(AVFILTER_PATH avfilter ${FFMPEG_INSTALL}/lib)
find_library(AVFORMAT_PATH avformat ${FFMPEG_INSTALL}/lib)
find_library(POSTPROC_PATH postproc ${FFMPEG_INSTALL}/lib)
find_library(SWSCALE_PATH swscale ${FFMPEG_INSTALL}/lib)
find_library(SWRESAMPLE_PATH swresample ${FFMPEG_INSTALL}/lib)
find_library(AVCODEC_PATH avcodec ${FFMPEG_INSTALL}/lib)
find_library(AVUTIL_PATH avutil ${FFMPEG_INSTALL}/lib)
find_library(X264_PATH x264 ${X264_INSTALL}/lib)
find_library(X265_PATH x265 ${X265_INSTALL}/lib)
find_library(LAME_PATH mp3lame ${LAME_INSTALL}/lib)
find_library(HEIF_READER heif_static ${HEIF_ROOT}/build/lib)
find_library(HEIF_WRITER heif_writer_static ${HEIF_ROOT}/build/lib)
find_library(HEIFPP heifpp ${HEIF_ROOT}/build/lib)
find_library(EXIV2 exiv2 ${EXIV2_INSTALL}/lib)

#add_library(x265
#            STATIC
#            IMPORTED)
#set_target_properties(x265
#        PROPERTIES IMPORTED_LOCATION
#        ${X265_INSTALL}/lib/libx265.so)


SET(LIBS ${AVFILTER_PATH}
         ${AVFORMAT_PATH}
         ${POSTPROC_PATH}
         ${SWSCALE_PATH}
         ${AVCODEC_PATH}
         ${SWRESAMPLE_PATH}
         ${AVUTIL_PATH}
         z
         lzma
         stdc++
         m
         rt
         ${X264_PATH}
         ${X265_PATH}
         dl
         numa
         ${EXIV2}
         ${LAME_PATH}
         ${LIBTCIFNAME}
         ${HEIFPP}
         ${HEIF_READER}
         ${HEIF_WRITER}
         pthread)

#link lib
TARGET_LINK_LIBRARIES(${PRONAME} ${LIBS})
