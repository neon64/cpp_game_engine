cmake_minimum_required(VERSION 3.17)
project(game_engine)

set(CMAKE_CXX_STANDARD 20)

#if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#    conan_basic_setup(TARGETS)
#else()
#    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
#endif()

function(add_shader name output_name)
    message("shader_codegen ${name} ${CMAKE_SOURCE_DIR}/res/shaders/${name}.vert ${CMAKE_SOURCE_DIR}/res/shaders/${name}.frag ${CMAKE_SOURCE_DIR}/gen/shaders/${output_name}.h --specialize \"${ARGN}\"")
    add_custom_command(
            OUTPUT  ${CMAKE_SOURCE_DIR}/gen/shaders/${output_name}.h ${CMAKE_SOURCE_DIR}/gen/shaders/${output_name}.cpp
            COMMAND shader_codegen ${output_name} ${CMAKE_SOURCE_DIR}/res/shaders/${name}.vert ${CMAKE_SOURCE_DIR}/res/shaders/${name}.frag ${CMAKE_SOURCE_DIR}/gen/shaders/${output_name}.h --specialize "\"${ARGN}\""
            DEPENDS res/shaders/${name}.vert res/shaders/${name}.frag src/codegen/shader_codegen.cpp src/codegen/glsl_to_cpp.cpp src/codegen/glsl_to_cpp.h
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set(shader_files gen/shaders/${output_name}.cpp ${shader_files} PARENT_SCOPE)
endfunction(add_shader)

add_shader(fullscreen fullscreen)
add_shader(lighting/all lighting_test NUM_LIGHTS=1 USE_COLOR_TEXTURE HAS_TEXTURE_COORDINATE USE_NORMAL_MAP)
add_shader(textured textured)

add_executable(shader_codegen src/codegen/shader_codegen.cpp src/codegen/glsl_to_cpp.cpp)

message("Generated shader files: ${shader_files}")

add_executable(game_engine
    src/main.cpp src/glad.c
        src/graphics/Shader.cpp src/graphics/Program.cpp
    src/errors.cpp src/graphics/OpenGLContext.cpp
    src/Window.cpp src/graphics/ColorRGBA.cpp
        src/graphics/commands.cpp src/graphics/texturing.cpp src/loader/cache.cpp src/lighting.cpp src/graphics/pipeline.cpp src/graphics/VertexArray.cpp src/graphics/OpenGLResource.cpp src/graphics/buffer.cpp
        src/Camera.cpp src/loader/stb_image.cpp src/graphics/RenderTarget.cpp src/loader/texture.cpp src/loader/models.cpp src/loader/shaders.cpp ${shader_files})

# include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(glslang)

add_definitions(-DLOGURU_WITH_STREAMS=1)

# workaround for missing symbols issue (https://github.com/microsoft/vcpkg/issues/9918)
find_library(ASSIMP_ZLIB_LIBRARY z)
find_library(ASSIMP_IRRXML_LIBRARY IrrXML)
message("ASSIMP_ZLIB_LIBRARY   = " "${ASSIMP_ZLIB_LIBRARY}")
message("ASSIMP_IRRXML_LIBRARY = " "${ASSIMP_IRRXML_LIBRARY}")

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(loguru CONFIG REQUIRED)

target_link_libraries(game_engine glm glfw dl assimp::assimp loguru
        ${ASSIMP_ZLIB_LIBRARY}
        ${ASSIMP_IRRXML_LIBRARY})

# fixes missing 'Threads::Threads' message that comes from glslang (https://github.com/microsoft/vcpkg/issues/7693)
include(CMakeFindDependencyMacro)
find_dependency(Threads)

target_link_libraries(shader_codegen PRIVATE glslang fmt::fmt loguru)