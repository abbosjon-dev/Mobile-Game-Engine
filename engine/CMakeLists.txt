cmake_minimum_required(VERSION 3.22)
project(ZaminEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Shader kompilyatsiyasi (GLSL -> SPIR-V, C massiv sifatida) ----
find_program(GLSLC glslc
    HINTS
        ${ANDROID_NDK}/shader-tools/linux-x86_64
        ${ANDROID_NDK}/shader-tools/darwin-x86_64
        ${ANDROID_NDK}/shader-tools/windows-x86_64
        $ENV{VULKAN_SDK}/bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc topilmadi (NDK shader-tools yoki Vulkan SDK kerak)")
endif()

set(ZAMIN_SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${ZAMIN_SHADER_OUT})
set(ZAMIN_SHADERS basic.vert basic.frag grid.vert grid.frag)
set(ZAMIN_SHADER_OUTPUTS)
foreach(shader ${ZAMIN_SHADERS})
    set(src ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader})
    set(out ${ZAMIN_SHADER_OUT}/${shader}.spv.inc)
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${GLSLC} --target-env=vulkan1.1 -O -mfmt=num -o ${out} ${src}
        DEPENDS ${src}
        COMMENT "glslc ${shader}")
    list(APPEND ZAMIN_SHADER_OUTPUTS ${out})
endforeach()
add_custom_target(zamin_shaders DEPENDS ${ZAMIN_SHADER_OUTPUTS})

# ---- Engine kutubxonasi ----
add_library(zamin_engine STATIC
    src/core/Log.cpp
    src/core/Time.cpp
    src/core/Engine.cpp
    src/rhi/vulkan/VulkanRenderer.cpp
)
add_dependencies(zamin_engine zamin_shaders)
target_include_directories(zamin_engine
    PUBLIC include
    PRIVATE ${ZAMIN_SHADER_OUT})
target_compile_options(zamin_engine PRIVATE -Wall -Wextra -Wno-missing-field-initializers)

if(ANDROID)
    target_link_libraries(zamin_engine PUBLIC vulkan android log)
else()
    find_package(Vulkan REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(zamin_engine PUBLIC Vulkan::Vulkan Threads::Threads)
endif()
