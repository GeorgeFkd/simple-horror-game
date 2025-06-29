cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

project(OpenGLApp LANGUAGES CXX)

option(BUILD_TESTS "Build Tests(Install Catch2 Dependency)" OFF)

set(CMAKE_CXX_STANDARD 17)



set(SOURCES
    src/main.cpp
    src/SceneManager.cpp
    src/TextRenderer.cpp
    src/OBJLoader.cpp
    src/Camera.cpp
    src/Model.cpp
    src/Shader.cpp
    src/data-structures/Light.cpp
        src/Group.cpp
        src/Monster.cpp
)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(Catch2)
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

target_include_directories(opengl_app PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_mixer_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    /usr/include/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/src   # So headers like Renderer.h and OBJLoader.h are found
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
    ${TIFF_INCLUDE_DIR}
)

target_link_libraries(opengl_app PRIVATE
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_mixer_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    TIFF::TIFF
    GLEW::glew
    glm::glm
)

##############
# TEST FUNCTIONALITY OF OBJECT LOADER
##############
add_executable(obj_loader src/OBJLoaderMain.cpp src/OBJLoader.cpp)

target_include_directories(obj_loader PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    /usr/include/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
        ${TIFF_INCLUDE_DIR}
)

target_link_libraries(obj_loader PRIVATE
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
        TIFF::TIFF
    GLEW::glew
    glm::glm
)

target_compile_definitions(obj_loader PRIVATE DEBUG_OBJLOADER)

##############
# ASSETS HERE
##############

# Set shader source and destination dirs
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(SHADER_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")
# Collect all shader files (.vert, .frag, etc.)
file(GLOB SHADERS "${SHADER_SRC_DIR}/*.*")

# Copy each shader file into build/assets/shaders/

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_SRC_DIR}" "${SHADER_DST_DIR}"
    COMMENT "Copying shader files..."
)

add_dependencies(opengl_app copy_shaders)

# Copy .obj/mtl model files into build/assets/models/
set(MODEL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
set(MODEL_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/models")

file(GLOB_RECURSE ALL_MODEL_FILES
    "${MODEL_SRC_DIR}/*.obj"
    "${MODEL_SRC_DIR}/*.mtl"
)

add_custom_target(copy_models ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODEL_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${MODEL_SRC_DIR}" "${MODEL_DST_DIR}"
    COMMENT "Copying model files..."
)

set(TEXTURES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures")
set(TEXTURES_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/textures")

file(GLOB_RECURSE ALL_TEXTURE_FILES
    "${TEXTURES_SRC_DIR}/*.jpg"
)

set(MUSIC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio")
set(MUSIC_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/audio")

file(GLOB_RECURSE ALL_AUDIO_FILES
    "${MUSIC_SRC_DIR}/*.mp3"
)

add_custom_target(copy_audio ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MUSIC_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${MUSIC_SRC_DIR}" "${MUSIC_DST_DIR}"

)


add_custom_target(copy_textures ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEXTURES_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${TEXTURES_SRC_DIR}" "${TEXTURES_DST_DIR}"
    COMMENT "Copying texture files..."

)


set(FONTS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
set(FONTS_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/fonts")
file(GLOB_RECURSE ALL_FONT_FILES
    "${FONTS_SRC_DIR}/*.ttf"
)



add_custom_target(copy_fonts ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FONTS_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${FONTS_SRC_DIR}" "${FONTS_DST_DIR}"
    COMMENT "Copying font files..."

)


add_dependencies(opengl_app copy_fonts)
add_dependencies(opengl_app copy_models)
add_dependencies(opengl_app copy_textures)
add_dependencies(opengl_app copy_audio)

