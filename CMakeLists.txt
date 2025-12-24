cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

project(OpenGLApp LANGUAGES CXX)

option(BUILD_TESTS "Build Tests(Installs GoogleTest Dependency)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)

FetchContent_MakeAvailable(googletest)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
# find_package(Catch2)
find_package(TIFF REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer)
find_package(Freetype REQUIRED)

if(NOT Freetype_FOUND)
    message(FATAL "Did not find freetype, install it using your package manager")
endif()

if(NOT SDL2_mixer_FOUND) 
    message(FATAL "Did not find sdl2 mixer")
endif()

if (NOT TIFF_FOUND)
    message(STATUS "Did not find TIFF, .tiff files wont load properly")
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

add_library(obj_loader_lib STATIC src/OBJLoader.cpp)
target_include_directories(obj_loader_lib 
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
PRIVATE ${OPENGL_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} /usr/include/glm
)

target_link_libraries(obj_loader_lib PRIVATE
    ${OPENGL_LIBRARIES}
    TIFF::TIFF
    GLEW::glew
    glm::glm
)


set(TEXT_HANDLER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures)
message(STATUS "Including ${TEXT_HANDLER_INCLUDES} for text handler")
add_library(text_handler STATIC src/TextRenderer.cpp)
target_include_directories(text_handler PRIVATE 
${TEXT_HANDLER_INCLUDES} 
${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(text_handler PRIVATE ${OPENGL_LIBRARIES} GLEW::GLEW glm::glm ${FREETYPE_LIBRARIES})


add_library(opengl_backend STATIC src/Shader.cpp)
target_include_directories(opengl_backend 
PRIVATE 
${OPENGL_INCLUDE_DIR} 
${GLEW_INCLUDE_DIR} 
/usr/include/glm 
${CMAKE_CURRENT_SOURCE_DIR}/src 
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
)

target_link_libraries(opengl_backend PRIVATE
${OPENGL_LIBRARIES} 
GLEW::GLEW 
glm::glm 
)

add_library(renderer STATIC src/Camera.cpp src/Model.cpp src/GameState.cpp src/Group.cpp src/Monster.cpp src/data-structures/Light.cpp src/SceneManager.cpp)

target_include_directories(renderer PUBLIC 
${CMAKE_CURRENT_SOURCE_DIR}/src 
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
${SDL2_INCLUDE_DIRS}
${SDL2_mixer_INCLUDE_DIRS}
/usr/include/glm
)

target_link_libraries(renderer PRIVATE 
${SDL2_LIBRARIES}
${SDL2_mixer_LIBRARIES}
glm::glm
opengl_backend
obj_loader_lib 
text_handler
)


add_executable(opengl_app src/main.cpp)

target_include_directories(opengl_app PRIVATE
    /usr/include/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/src   
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
)

target_link_libraries(opengl_app PRIVATE
    glm::glm
    renderer
)


##############
# ASSETS HERE
##############
function(copy_assets TARGET NAME SRC_DIR DST_DIR)
    add_custom_target(copy_${NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${DST_DIR}"
        COMMENT "Copying ${NAME} assets..."
    )

    add_dependencies(${TARGET} copy_${NAME})
endfunction()

copy_assets(
    opengl_app
    shaders
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
)

copy_assets(
    opengl_app
    models
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/models"
)

copy_assets(
    opengl_app
    textures
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/textures"
)

copy_assets(
    opengl_app
    audio
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/audio"
)

copy_assets(
    opengl_app
    fonts
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/fonts"
)


if (${BUILD_TESTS})
    message(STATUS "building tests using googletest library")
    add_subdirectory(src/tests)
else()
    message(STATUS "not building tests, use -DBUILD_TESTS=ON TO BUILD TESTS")
endif()

