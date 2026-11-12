cmake_minimum_required(VERSION 3.22)
project(zamin_snapshot LANGUAGES CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../engine ${CMAKE_CURRENT_BINARY_DIR}/engine)
add_executable(zamin_snapshot main.cpp)
find_package(ZLIB REQUIRED)
target_link_libraries(zamin_snapshot PRIVATE zamin_engine ZLIB::ZLIB)
