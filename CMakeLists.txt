cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(OpenGLApp)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES src/main.cpp)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(opengl_app ${SOURCES})

target_include_directories(opengl_app PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

target_link_libraries(opengl_app
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${GLEW_LIBRARIES}
)
