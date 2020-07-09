cmake_minimum_required(VERSION 3.17)
project(game_engine)

set(CMAKE_CXX_STANDARD 20)

#if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#    conan_basic_setup(TARGETS)
#else()
#    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
#endif()

function(add_shader name)
    add_custom_command(
            OUTPUT  ${CMAKE_SOURCE_DIR}/gen/shaders/${name}.h ${CMAKE_SOURCE_DIR}/gen/shaders/${name}.cpp
            COMMAND shader_codegen ${name} ${CMAKE_SOURCE_DIR}/res/shaders/${name}.vert ${CMAKE_SOURCE_DIR}/res/shaders/${name}.frag ${CMAKE_SOURCE_DIR}/gen/shaders/${name}.h
            DEPENDS res/shaders/${name}.vert res/shaders/${name}.frag src/loader/shader_codegen.cpp
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set(shader_files gen/shaders/${name}.cpp ${shader_files} PARENT_SCOPE)
endfunction(add_shader)

add_shader(fullscreen)
add_shader(textured)

add_executable(shader_codegen src/loader/shader_codegen.cpp src/loader/glsl_to_cpp.cpp)

message("Generated shader files: ${shader_files}")

add_executable(game_engine
    src/main.cpp src/glad.c
    src/Shader.cpp src/Program.cpp
    src/errors.cpp src/OpenGLContext.cpp
    src/Window.cpp src/ColorRGBA.cpp
    src/pipelines/textured.cpp
    src/commands.cpp src/texturing.cpp src/pipeline.cpp src/VertexArray.cpp src/OpenGLResource.cpp src/buffer.cpp
    src/Camera.cpp src/loader/stb_image.cpp src/RenderTarget.cpp src/loader/texture.cpp src/loader/models.cpp ${shader_files})

# include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(glslang)

# workaround for missing symbols issue (https://github.com/microsoft/vcpkg/issues/9918)
find_library(ASSIMP_ZLIB_LIBRARY z)
find_library(ASSIMP_IRRXML_LIBRARY IrrXML)
message("ASSIMP_ZLIB_LIBRARY   = " "${ASSIMP_ZLIB_LIBRARY}")
message("ASSIMP_IRRXML_LIBRARY = " "${ASSIMP_IRRXML_LIBRARY}")

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

target_link_libraries(game_engine glm glfw dl assimp::assimp
        ${ASSIMP_ZLIB_LIBRARY}
        ${ASSIMP_IRRXML_LIBRARY})

# fixes missing 'Threads::Threads' message that comes from glslang (https://github.com/microsoft/vcpkg/issues/7693)
include(CMakeFindDependencyMacro)
find_dependency(Threads)

target_link_libraries(shader_codegen PRIVATE glslang fmt::fmt)