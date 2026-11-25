# Engine unit testlari: GPU talab qilmaydi (faqat matematika, kamera, sahna).
# cmake -S engine/tests -B build/tests && cmake --build build/tests && ctest --test-dir build/tests
cmake_minimum_required(VERSION 3.22)
project(zamin_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(zamin_tests EngineTests.cpp ${ENGINE}/src/scene/Scene.cpp)
target_include_directories(zamin_tests PRIVATE ${ENGINE}/include)
target_compile_options(zamin_tests PRIVATE -Wall -Wextra -Werror -Wno-missing-field-initializers)

enable_testing()
add_test(NAME zamin_tests COMMAND zamin_tests)
