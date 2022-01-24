cmake_minimum_required(VERSION 2.8.11)
project(timidity_sdl C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../external/audio_codec_common.cmake)

find_package(SDL2 REQUIRED)

add_library(timidity STATIC
    common.c
    instrum.c
    mix.c
    output.c
    playmidi.c
    readmidi.c
    resample.c
    tables.c
    timidity.c
)

target_include_directories(timidity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(timidity PUBLIC ${SDL2_INCLUDE_DIRS})

install(TARGETS timidity
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        INCLUDES DESTINATION "include")

install(FILES
        timidity.h
        DESTINATION include/)
