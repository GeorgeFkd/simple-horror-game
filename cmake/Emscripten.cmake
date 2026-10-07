# Web (Emscripten/WebAssembly) build support.
#
# Included from the top-level CMakeLists.txt when configuring with emcmake. The native
# find_package() calls are replaced by Emscripten ports (SDL2, SDL2_mixer, FreeType), header-only
# glm fetched from GitHub, Emscripten's built-in GLEW/OpenGL emulation, and libtiff built from
# source (it has no Emscripten port). The rest of CMakeLists.txt keeps linking the usual
# target names (SDL2::SDL2, TIFF::TIFF, ...) so the native build logic is untouched.

set(WEB_PORT_FLAGS
    "-sUSE_SDL=2"
    "-sUSE_SDL_MIXER=2"
    "-sSDL2_MIXER_FORMATS=[\"mp3\"]"
    "-sUSE_FREETYPE=1")
# Ports must be requested both when compiling (headers) and when linking (libraries). They are set
# globally because their headers are reached transitively (e.g. SceneManager.h -> SDL_mixer.h).
add_compile_options(${WEB_PORT_FLAGS})

set(WEB_MAX_SHADOW_MAP_SIZE
    1024
    CACHE STRING "Largest shadow map side in the web build (lower = faster on weak GPUs)")
add_compile_definitions(WEB_MAX_SHADOW_MAP_SIZE=${WEB_MAX_SHADOW_MAP_SIZE})
add_link_options(${WEB_PORT_FLAGS})

# Placeholder targets for dependencies that the ports / Emscripten itself provide.
foreach(dep OpenGL::GL OpenGL::GLX GLEW::GLEW SDL2::SDL2 SDL2_mixer::SDL2_mixer
            Freetype::Freetype)
  if(NOT TARGET ${dep})
    add_library(${dep} INTERFACE IMPORTED)
  endif()
endforeach()
set(OpenGL_FOUND TRUE)
set(GLEW_FOUND TRUE)
set(SDL2_FOUND TRUE)
set(SDL2_mixer_FOUND TRUE)
set(Freetype_FOUND TRUE)

# glm is header-only; fetch it since host headers must not leak into a wasm build.
include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE
  # only download, glm's own CMake project is not needed for a header-only include path
  SOURCE_SUBDIR do-not-add-subdirectory)
FetchContent_MakeAvailable(glm)
if(NOT TARGET glm::glm)
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                            "${glm_SOURCE_DIR}")
endif()

# libtiff has no Emscripten port but is portable C, so it is built from source. Only the codecs
# that need no extra libraries are enabled (LZW/PackBits cover the game's .tif textures).
set(tiff-tools OFF CACHE BOOL "" FORCE)
set(tiff-tests OFF CACHE BOOL "" FORCE)
set(tiff-contrib OFF CACHE BOOL "" FORCE)
set(tiff-docs OFF CACHE BOOL "" FORCE)
set(tiff-cxx OFF CACHE BOOL "" FORCE)
set(tiff-install OFF CACHE BOOL "" FORCE)
foreach(codec zlib libdeflate pixarlog jpeg old-jpeg jpeg12 jbig lerc lzma zstd webp)
  set(${codec} OFF CACHE BOOL "" FORCE)
endforeach()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  libtiff
  GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
  GIT_TAG v4.7.0
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(libtiff)
if(NOT TARGET TIFF::TIFF)
  add_library(TIFF::TIFF ALIAS tiff)
endif()
set(TIFF_FOUND TRUE)

# Collects every asset file the game needs at runtime: string literals "assets/..." in the sources,
# the .mtl files referenced by those .obj files, and the textures referenced by those .mtl files.
# Only these files get packaged, instead of the whole (~160MB) assets folder.
function(collect_web_assets OUT_VAR)
  file(GLOB_RECURSE game_sources ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)
  set(assets "")
  foreach(src ${game_sources})
    file(STRINGS ${src} lines REGEX "\"assets/[^\"]+\"")
    foreach(line ${lines})
      string(REGEX MATCHALL "\"assets/[^\"]+\"" literals "${line}")
      foreach(lit ${literals})
        string(REPLACE "\"" "" lit "${lit}")
        if(EXISTS ${CMAKE_SOURCE_DIR}/${lit} AND NOT IS_DIRECTORY
                                                  ${CMAKE_SOURCE_DIR}/${lit})
          list(APPEND assets ${lit})
        endif()
      endforeach()
    endforeach()
  endforeach()
  list(REMOVE_DUPLICATES assets)

  set(result ${assets})
  foreach(asset ${assets})
    if(NOT asset MATCHES "\\.obj$")
      continue()
    endif()
    get_filename_component(obj_dir ${asset} DIRECTORY)
    file(STRINGS ${CMAKE_SOURCE_DIR}/${asset} mtllibs REGEX "^[ \t]*mtllib[ \t]")
    foreach(mtllib ${mtllibs})
      string(REGEX REPLACE "^[ \t]*mtllib[ \t]+([^ \t#]+).*$" "\\1" mtl "${mtllib}")
      set(mtl "${obj_dir}/${mtl}")
      if(NOT EXISTS ${CMAKE_SOURCE_DIR}/${mtl})
        continue()
      endif()
      list(APPEND result ${mtl})
      # Same keys as OBJLoader::read_mtllib
      file(STRINGS ${CMAKE_SOURCE_DIR}/${mtl} maps
           REGEX "^[ \t]*(map_Ka|map_Kd|map_Ks|bump|map_bump)[ \t]")
      foreach(map ${maps})
        string(REGEX REPLACE "^[ \t]*[A-Za-z_]+[ \t]+(.*[^ \t\r])[ \t\r]*$" "\\1" tex
                             "${map}")
        if(EXISTS ${CMAKE_SOURCE_DIR}/${obj_dir}/${tex})
          list(APPEND result "${obj_dir}/${tex}")
        endif()
      endforeach()
    endforeach()
  endforeach()
  list(REMOVE_DUPLICATES result)
  set(${OUT_VAR}
      ${result}
      PARENT_SCOPE)
endfunction()

function(configure_web_executable TARGET)
  collect_web_assets(web_assets)
  list(LENGTH web_assets num_assets)
  message(STATUS "Packaging ${num_assets} asset files (+ shaders) for the web build")

  # SHELL: keeps each flag/value pair together, CMake would otherwise de-duplicate the repeated
  # --preload-file flags
  set(preload_flags
      "SHELL:--preload-file \"${CMAKE_SOURCE_DIR}/assets/shaders@/assets/shaders\"")
  foreach(asset ${web_assets})
    list(APPEND preload_flags
         "SHELL:--preload-file \"${CMAKE_SOURCE_DIR}/${asset}@/${asset}\"")
  endforeach()

  set_target_properties(${TARGET} PROPERTIES SUFFIX ".html")
  target_link_options(
    ${TARGET}
    PRIVATE
    "-sMIN_WEBGL_VERSION=2"
    "-sMAX_WEBGL_VERSION=2"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=256MB"
    "-sSTACK_SIZE=8MB"
    "SHELL:--shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\""
    ${preload_flags})
  set_property(
    TARGET ${TARGET}
    APPEND
    PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/web/shell.html)
endfunction()
