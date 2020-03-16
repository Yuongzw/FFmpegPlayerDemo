
cmake_minimum_required(VERSION 3.4.1)

include_directories(include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI}")
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/*.cpp)
add_library(
        player
        SHARED
        #        ${SOURCE}
        #
        VideoChannel.cpp
        AudioChannel.cpp
        JavaCallHelper.cpp
        FFmpegControl.cpp
        native-lib.cpp
)

find_library( # Sets the name of the path variable.
        log-lib
        log)

target_link_libraries(
        player
        avfilter avformat avcodec avutil swresample swscale
        android
        z
        OpenSLES
        ${log-lib})