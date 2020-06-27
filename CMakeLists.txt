cmake_minimum_required(VERSION 3.17)
project(game_engine)

set(CMAKE_CXX_STANDARD 20)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else()
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

add_executable(game_engine main.cpp src/glad.c src/Shader.cpp src/Program.cpp src/errors.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)

message("${CONAN_LIBS}")

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(game_engine glfw dl CONAN_PKG::assimp CONAN_PKG::zlib)