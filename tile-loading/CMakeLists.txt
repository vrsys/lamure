cmake_minimum_required(VERSION 3.8)
project(tile_provider)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DDEBUGE)

set(CMAKE_CXX_FLAGS -pthread)

set(SOURCE_FILES main.cpp PageProvider.cpp PageProvider.h Request.cpp Request.h definitions.h RequestQueue.cpp RequestQueue.h QueuedProcessor.cpp QueuedProcessor.h PagedBuffer.cpp PagedBuffer.h Decompressor.cpp Decompressor.h PageCache.cpp PageCache.h TextureAtlas.cpp TextureAtlas.h)
add_executable(tile_provider ${SOURCE_FILES})