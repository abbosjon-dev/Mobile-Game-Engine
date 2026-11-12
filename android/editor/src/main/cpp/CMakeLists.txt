cmake_minimum_required(VERSION 3.22)
project(zamin_editor_native LANGUAGES CXX)

set(ZAMIN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
add_subdirectory(${ZAMIN_ROOT}/engine ${CMAKE_CURRENT_BINARY_DIR}/engine)

add_library(zamin SHARED
    ${ZAMIN_ROOT}/engine/src/platform/android/JniBridge.cpp)
target_link_libraries(zamin PRIVATE zamin_engine)
