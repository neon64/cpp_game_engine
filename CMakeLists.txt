cmake_minimum_required(VERSION 3.17)
project(game_engine)

set(CMAKE_CXX_STANDARD 20)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else()
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

function(make_includable input_file output_file)
#    add_custom_command(
#        COMMAND make_includable res/shaders/${name} gen/shaders/${name}
#        DEPENDS res/shaders/${name}
#        OUTPUTS gen/shaders/${name}
#        COMMENT "Generating code for ${name}."
#    )
    file(READ ${input_file} content)
    set(delim "for_c++_include")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

#add_custom_target(shaders ALL
#        DEPENDS gen/shaders/basicTest.vert)

make_includable(res/shaders/basicTest.frag gen/shaders/basicTest.frag)
make_includable(res/shaders/basicTest.vert gen/shaders/basicTest.vert)
make_includable(res/shaders/fullscreen.vert gen/shaders/fullscreen.vert)
make_includable(res/shaders/fullscreen.frag gen/shaders/fullscreen.frag)

add_executable(game_engine
    src/main.cpp src/glad.c
    src/Shader.cpp src/Program.cpp
    src/errors.cpp src/OpenGLContext.cpp
    src/Window.cpp src/ColorRGBA.cpp
    src/commands.cpp src/texturing.cpp src/pipeline.cpp src/VertexArray.cpp src/OpenGLResource.cpp src/Buffer.cpp src/Camera.cpp src/stb_image.cpp src/RenderTarget.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)

message("${CONAN_LIBS}")

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(game_engine glfw dl CONAN_PKG::assimp CONAN_PKG::zlib)