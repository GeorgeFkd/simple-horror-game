cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(OpenGLApp LANGUAGES CXX)

option(BUILD_TESTS "Build Tests(Install Catch2 Dependency)" OFF)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES src/main.cpp) 

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(Catch2)

if (NOT Catch2_FOUND)
    message(STATUS "Did not find Catch2, setting BUILD_TESTS option to OFF")
    set(BUILD_TESTS OFF)
endif()


if (NOT GLEW_FOUND)
    message(ERROR "ERROR: Glew not found")
endif()

if (NOT OpenGL_FOUND)
    message(ERROR "Opengl was not found")
endif()

if (NOT SDL2_FOUND)
    message(ERROR "SDL2 was not found")
endif()


if (${BUILD_TESTS})
    message(STATUS "building tests...")
    add_subdirectory(src/tests)
else()
    message(STATUS "not building tests, use -DBUILD_TESTS=ON TO BUILD TESTS")
endif()


add_executable(opengl_app ${SOURCES})
add_executable(obj_loader src/OBJLoaderMain.cpp src/OBJLoader.cpp)
target_include_directories(obj_loader PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    /usr/include/glm
)

target_link_libraries(obj_loader PRIVATE
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    GLEW::glew
    glm::glm
)

target_include_directories(opengl_app PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    /usr/include/glm
)

target_link_libraries(opengl_app PRIVATE
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    GLEW::glew
    glm::glm
)
