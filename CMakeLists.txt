cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

project(OpenGLApp LANGUAGES C CXX)

option(BUILD_TESTS "Build Tests(Installs GoogleTest Dependency)" OFF)
option(BUILD_SDL2 "Build SDL2" OFF)
option(ENABLE_ASSET_PIPELINES "Enable assets pipelines" OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE
        STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()
if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
  set(USING_VCPKG TRUE)
else()
  set(USING_VCPKG FALSE)
endif()

if(BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
  )

  FetchContent_MakeAvailable(googletest)
endif()

find_package(OpenGL REQUIRED)

if(NOT BUILD_SDL2)
  find_package(SDL2 REQUIRED)
else()
  message(STATUS "Building SDL2 from source")
endif()

find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(TIFF REQUIRED)
if(LINUX AND (NOT USING_VCPKG))
  message(STATUS "using pkg config to find sdl2_mixer")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer)
else()
  message(STATUS "not using pkg config to find sdl2_mixer")
  find_package(SDL2_mixer CONFIG REQUIRED)
endif()
find_package(Freetype REQUIRED)

if(NOT Freetype_FOUND)
  message(FATAL "Did not find freetype, install it using your package manager")
endif()

if(NOT SDL2_mixer_FOUND)
  message(FATAL
          "Did not find sdl2 mixer, install it using your package manager")
endif()

if(NOT TIFF_FOUND)
  message(ERROR "Did not find TIFF, .tiff files wont load properly")
endif()

if(NOT GLEW_FOUND)
  message(ERROR "ERROR: Glew not found")
endif()

if(NOT OpenGL_FOUND)
  message(ERROR "Opengl was not found")
endif()

if(NOT SDL2_FOUND)
  message(ERROR "SDL2 was not found")
endif()

set(TEXT_HANDLER_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer)
message(STATUS "Including ${TEXT_HANDLER_INCLUDES} for text handler")
add_library(text_handler STATIC src/renderer/TextRenderer.cpp)
target_include_directories(text_handler PRIVATE ${TEXT_HANDLER_INCLUDES})
target_link_libraries(
  text_handler PRIVATE glm::glm Freetype::Freetype
                       $<$<AND:$<BOOL:${LINUX}>,$<BOOL:${VCPKG}>>:PNG::PNG>)

add_library(opengl_backend STATIC src/renderer/Shader.cpp)
target_include_directories(
  opengl_backend
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
          ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer)

target_link_libraries(
  opengl_backend PRIVATE OpenGL::GL $<$<BOOL:${LINUX}>:OpenGL::GLX> GLEW::GLEW
                         glm::glm)

add_library(model_loader STATIC src/loaders/OBJLoader.cpp
                                src/loaders/ModelLoader.cpp)
target_include_directories(
  model_loader
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
          ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/loaders)

target_link_libraries(model_loader PRIVATE TIFF::TIFF glm::glm)

add_library(
  scene_objects STATIC src/scene_objects/Model.cpp src/scene_objects/Group.cpp
                       src/scene_objects/Light.cpp src/scene_objects/MData.cpp)

target_include_directories(
  scene_objects
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
          ${CMAKE_CURRENT_SOURCE_DIR}/src/scene_objects)

target_link_libraries(scene_objects PRIVATE glm::glm TIFF::TIFF)

add_library(
  opengl_renderer STATIC
  src/renderer/Renderer.cpp src/renderer/GPUMeshOpenGL.cpp
  src/renderer/GPULight.cpp)
target_include_directories(
  opengl_renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                          ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures)

target_link_libraries(opengl_renderer PRIVATE opengl_backend glm::glm
                                              scene_objects)

add_library(audioplayer STATIC src/audio/AudioPlayer.cpp)
target_include_directories(audioplayer PRIVATE ${SDL2_mixer_INCLUDE_DIRS})

add_library(game STATIC src/game/Camera.cpp src/game/GameState.cpp
                        src/scene_objects/Monster.cpp src/game/SceneManager.cpp)

target_include_directories(
  game
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
          ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
          ${CMAKE_CURRENT_SOURCE_DIR}/src/game
          ${CMAKE_CURRENT_SOURCE_DIR}/src/scene_objects
          ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
          ${SDL2_INCLUDE_DIRS})

if(ENABLE_ASSET_PIPELINES)
  message(STATUS "Asset pipelines are enabled")
  # https://github.com/Sirraide/clang-minimal-example/blob/master/README.md
  if(NOT DEFINED LLVM_PATH)
    # on ubuntu apt install llvm-19 llvm-19-dev
    find_package(LLVM 19.1 REQUIRED)
    find_package(Clang 19.1 REQUIRED)
  else()
    set(LLVM_DIR "${LLVM_PATH}/lib/cmake/llvm")
    set(Clang_DIR "${LLVM_PATH}/lib/cmake/clang")
    find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH)
    find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH)
  endif()
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} in ${LLVM_INCLUDE_DIRS}")
  message(STATUS "Found Clang in ${CLANG_INCLUDE_DIRS}")
  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
  list(APPEND CMAKE_MODULE_PATH "${CLANG_CMAKE_DIR}")
  include(AddLLVM)
  include(AddClang)
  add_clang_executable(asset_extractor src/AssetExtractor.cpp)

  target_include_directories(asset_extractor SYSTEM
                             PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
  target_link_libraries(
    asset_extractor
    PRIVATE LLVMSupport
            LLVMFrontendOpenMP
            clangTooling
            clangASTMatchers
            clangAST
            clangBasic
            clangFrontend
            clangSerialization)
  target_compile_definitions(
    asset_extractor
    PRIVATE "CLANG_PATH=\"~/programming/llvm-project/build/bin/clang++\"")
  set(MENTIONED_ASSETS_FILENAME mentioned_assets.txt)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${MENTIONED_ASSETS_FILENAME}
    COMMAND asset_extractor ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/${MENTIONED_ASSETS_FILENAME} ${CMAKE_BINARY_DIR}
    DEPENDS asset_extractor)
  add_custom_target(run_asset_extractor
                    DEPENDS ${CMAKE_BINARY_DIR}/${MENTIONED_ASSETS_FILENAME})

  set(REQUIRED_ASSETS_FILENAME required_assets.txt)
  add_executable(asset_dependent_files_finder src/AssetDependentFileFinder.cpp)
  target_include_directories(
    asset_dependent_files_finder
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(asset_dependent_files_finder PRIVATE model_loader
                                                             scene_objects)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${REQUIRED_ASSETS_FILENAME}
    COMMAND
      asset_dependent_files_finder
      ${CMAKE_BINARY_DIR}/${MENTIONED_ASSETS_FILENAME}
      ${CMAKE_BINARY_DIR}/${REQUIRED_ASSETS_FILENAME}
    DEPENDS asset_dependent_files_finder)
  add_dependencies(asset_dependent_files_finder run_asset_extractor)
  add_custom_target(run_asset_dependent_files_finder
                    DEPENDS ${CMAKE_BINARY_DIR}/${REQUIRED_ASSETS_FILENAME})

  add_executable(copy_asset_files src/CopyFilesForBuild.cpp)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/assets
    COMMAND
      copy_asset_files ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
      ${CMAKE_BINARY_DIR}/${REQUIRED_ASSETS_FILENAME}
    DEPENDS copy_asset_files)
  add_dependencies(copy_asset_files run_asset_dependent_files_finder)
  add_custom_target(run_copy_asset_files DEPENDS ${CMAKE_BINARY_DIR}/assets)
endif()

add_executable(opengl_app src/main.cpp)
if(MSVC)
  set_property(
    TARGET opengl_app PROPERTY MSVC_RUNTIME_LIBRARY
                               "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_include_directories(
  opengl_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
                     ${CMAKE_CURRENT_SOURCE_DIR}/src/data-structures)

if(LINUX)
  if(USING_VCPKG)
    message(STATUS "Linking SDL2_mixer and SDL with vcpkg style")
    target_link_libraries(
      audioplayer
      PRIVATE
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    )
    target_link_libraries(
      game
      PRIVATE SDL2::SDL2
              glm::glm
              opengl_renderer
              scene_objects
              model_loader
              text_handler
              audioplayer)

  else()
    target_link_libraries(audioplayer PRIVATE ${SDL2_mixer_LIBRARIES})

    target_link_libraries(
      game
      PRIVATE ${SDL2_LIBRARIES}
              glm::glm
              opengl_renderer
              scene_objects
              model_loader
              text_handler
              audioplayer)
  endif()
  target_link_libraries(opengl_app PRIVATE glm::glm game OpenGL::GLX)
else()
  target_link_libraries(
    game
    PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
            glm::glm
            opengl_renderer
            scene_objects
            model_loader
            text_handler
            audioplayer)
  target_link_libraries(
    opengl_app PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> glm::glm game)
endif()

if(ENABLE_ASSET_PIPELINES)
  add_dependencies(opengl_app run_copy_asset_files)
else()
  function(copy_assets TARGET NAME SRC_DIR DST_DIR)
    add_custom_target(
      copy_${NAME} ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${DST_DIR}"
      COMMENT "Copying ${NAME} assets...")

    add_dependencies(${TARGET} copy_${NAME})
  endfunction()

  copy_assets(
    opengl_renderer shaders "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")

  copy_assets(opengl_app models "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
              "${CMAKE_CURRENT_BINARY_DIR}/assets/models")

  copy_assets(opengl_app textures "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures"
              "${CMAKE_CURRENT_BINARY_DIR}/assets/textures")

  copy_assets(opengl_app audio "${CMAKE_CURRENT_SOURCE_DIR}/assets/audio"
              "${CMAKE_CURRENT_BINARY_DIR}/assets/audio")

  copy_assets(opengl_app fonts "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
              "${CMAKE_CURRENT_BINARY_DIR}/assets/fonts")
endif()

if(BUILD_TESTS)
  message(STATUS "building tests using googletest library")
  add_subdirectory(src/tests)
else()
  message(STATUS "not building tests, use -DBUILD_TESTS=ON TO BUILD TESTS")
endif()
