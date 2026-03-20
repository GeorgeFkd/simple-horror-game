cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

project(OpenGLApp LANGUAGES CXX)

option(BUILD_TESTS "Build Tests(Installs GoogleTest Dependency)" OFF)
option(BUILD_SDL2 "Build SDL2" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)

FetchContent_MakeAvailable(googletest)

find_package(OpenGL REQUIRED)


if(NOT BUILD_SDL2)
    find_package(SDL2 REQUIRED)
else() 
    message(STATUS "Building SDL2 from source")
endif()


# find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(TIFF REQUIRED)
if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer)
else() 
    find_package(SDL2_mixer CONFIG REQUIRED)
endif()
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




set(TEXT_HANDLER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer)
message(STATUS "Including ${TEXT_HANDLER_INCLUDES} for text handler")
add_library(text_handler STATIC src/renderer/TextRenderer.cpp)
target_include_directories(text_handler PRIVATE 
${TEXT_HANDLER_INCLUDES} 
${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(text_handler PRIVATE glm::glm ${FREETYPE_LIBRARIES})


add_library(opengl_backend STATIC src/renderer/Shader.cpp)
target_include_directories(opengl_backend 
PRIVATE 
${OPENGL_INCLUDE_DIR} 
${GLEW_INCLUDE_DIR} 
/usr/include/glm 
${CMAKE_CURRENT_SOURCE_DIR}/src 
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
)

target_link_libraries(opengl_backend PRIVATE
${OPENGL_LIBRARIES} 
GLEW::GLEW 
glm::glm 
)

add_library(model_loader STATIC src/loaders/OBJLoader.cpp src/loaders/ModelLoader.cpp)
target_include_directories(model_loader PRIVATE 
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures 
${CMAKE_CURRENT_SOURCE_DIR}/src
${CMAKE_CURRENT_SOURCE_DIR}/loaders
${TIFF_INCLUDE_DIR} /usr/include/glm
)

target_link_libraries(model_loader PRIVATE
    TIFF::TIFF
    glm::glm
)


add_library(scene_objects STATIC src/scene_objects/Model.cpp src/scene_objects/Group.cpp src/scene_objects/Light.cpp src/scene_objects/MData.cpp)

target_include_directories(scene_objects PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
${CMAKE_CURRENT_SOURCE_DIR}/src/scene_objects
${TIFF_INCLUDE_DIR}
/usr/include/glm
)

target_link_libraries(scene_objects PRIVATE
glm::glm
TIFF::TIFF
)


add_library(opengl_renderer STATIC src/renderer/Renderer.cpp src/renderer/GPUMeshOpenGL.cpp src/renderer/GPULight.cpp)
target_include_directories(opengl_renderer PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
/usr/include/glm
)

target_link_libraries(opengl_renderer PRIVATE
opengl_backend
glm::glm
scene_objects
)

add_library(game STATIC src/game/Camera.cpp  src/game/GameState.cpp  src/scene_objects/Monster.cpp src/game/SceneManager.cpp)




target_include_directories(game PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src 
${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
${CMAKE_CURRENT_SOURCE_DIR}/src/game
${CMAKE_CURRENT_SOURCE_DIR}/src/scene_objects
${SDL2_INCLUDE_DIRS}
${SDL2_mixer_INCLUDE_DIRS}
/usr/include/glm
)
add_executable(opengl_app src/main.cpp)

target_include_directories(opengl_app PRIVATE
    /usr/include/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/src   
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
)

if(LINUX)
    target_link_libraries(game PRIVATE 
            # SDL2::SDL2
            # SDL2_mixer::SDL2_mixer
        ${SDL2_LIBRARIES}
        ${SDL2_mixer_LIBRARIES}
        glm::glm
        opengl_renderer
        scene_objects
        model_loader
        text_handler
    )
    target_link_libraries(opengl_app PRIVATE
        glm::glm
        game
    )
else()
    target_link_libraries(game PRIVATE 
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
        glm::glm
        opengl_renderer
        scene_objects
        model_loader
        text_handler
    )
    target_link_libraries(opengl_app PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        glm::glm
        game
    )
endif()


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
    opengl_renderer
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

