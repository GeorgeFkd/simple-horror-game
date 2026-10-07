# Web (WebAssembly) build

The game can now be compiled to WebAssembly with Emscripten and runs in the browser on WebGL2.
The native desktop build is unchanged: every web-specific code path is behind `#ifdef __EMSCRIPTEN__`
or `if(EMSCRIPTEN)` in CMake, and the native libraries/implementations were not removed.

## Building and running

```sh
# once: install emsdk (https://emscripten.org/docs/getting_started/downloads.html)
git clone https://github.com/emscripten-core/emsdk.git && cd emsdk
./emsdk install latest && ./emsdk activate latest

# every shell
source /path/to/emsdk/emsdk_env.sh

cmake --preset web            # or: emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
python3 -m http.server -d build-web 8000   # pages must be served over http, not file://
# open http://localhost:8000/opengl_app.html, click the canvas to capture the mouse
```

Output: `opengl_app.html/.js/.wasm` plus `opengl_app.data` (the packaged assets).

Cache option: `-DWEB_MAX_SHADOW_MAP_SIZE=<n>` (default `1024`), see Performance below.

## Dependencies on the web

| Native                  | Web                                                                 |
|-------------------------|---------------------------------------------------------------------|
| SDL2                    | Emscripten port (`-sUSE_SDL=2`)                                     |
| SDL2_mixer              | Emscripten port with mp3 (`-sUSE_SDL_MIXER=2 -sSDL2_MIXER_FORMATS=["mp3"]`) |
| FreeType                | Emscripten port (`-sUSE_FREETYPE=1`)                                |
| GLEW / OpenGL 3.3 core  | Emscripten's GLEW emulation header / WebGL2 (OpenGL ES 3.0)         |
| glm                     | same library, fetched with FetchContent (header only)               |
| libtiff                 | same library (v4.7.0), no Emscripten port so it is built from source with FetchContent; only codecs without extra dependencies (LZW, PackBits, ...) are enabled |

All of this lives in `cmake/Emscripten.cmake`, which creates the same target names the rest of
`CMakeLists.txt` links against (`SDL2::SDL2`, `TIFF::TIFF`, `glm::glm`, ...), so the per-target
setup is shared between both builds.

## Changes

### Build system
- `CMakeLists.txt`: the `find_package` block is wrapped in `if(EMSCRIPTEN) include(cmake/Emscripten.cmake) else() ... endif()`;
  the web executable is configured by `configure_web_executable()` instead of copying assets.
- `CMakeLists.txt`: `opengl_renderer` now links `TIFF::TIFF` explicitly. `GPUMeshOpenGL.cpp` includes
  `tiffio.h` but the dependency was never declared; it only worked natively because the header is
  in a system include path.
- `cmake/Emscripten.cmake` (new): ports, placeholder targets, libtiff/glm fetching, WebGL2 link flags
  (`-sMIN/MAX_WEBGL_VERSION=2`, memory growth, 8MB stack) and asset packaging.
- Asset packaging: instead of the whole ~160MB `assets/` folder, only what the game uses is preloaded
  (~31MB): every `"assets/..."` string literal in `src/`, the `.mtl` files of those `.obj` files and
  the textures those `.mtl` files reference (same keys `OBJLoader` reads), plus `assets/shaders/`.
- `CMakePresets.json`: new `web` preset (needs `EMSDK` from `emsdk_env.sh`).
- `web/shell.html` (new): HTML page around the canvas. With `?log=1` it also POSTs stdout to `/log`,
  used to measure FPS from a script; it is off by default.
- `.gitignore`: `build-web/`.
- `.github/workflows/web_build.yml` (new): CI job that installs emsdk (pinned to the tested version,
  cached so the ports aren't rebuilt each run), builds with the `web` preset, checks the four output
  files exist and uploads them as a `web-build` artifact. Runs on pushes/PRs to `master`, alongside the
  existing native workflow.

### Rendering (WebGL2 differences)
- `src/renderer/GlPlatform.h` (new): constants that differ between GL 3.3 and WebGL2. The desktop values
  are exactly what was used before:
  - sized internal formats (`GL_DEPTH_COMPONENT32F`, `GL_R8`) for depth and font glyph textures;
  - `GL_CLAMP_TO_EDGE` instead of `GL_CLAMP_TO_BORDER` + border colour for spot light shadow maps
    (WebGL2 has no border colour; fragments outside a light's frustum sample the edge texel instead of "lit");
  - no geometry shaders.
- Point light shadows: WebGL2 has no geometry shaders or layered `glFramebufferTexture`, so on the web
  `Renderer::draw_light_depth` attaches one cube face at a time and draws it with
  `assets/shaders/web/depth_cube.{vert,frag}` (new, the geometry-shader-free equivalent of
  `depth_cube.{vert,geom,frag}`). The desktop path is unchanged.
- `Shader::load_file`: on the web the `#version 330 core` line is replaced by `#version 300 es` + default
  precisions, so the same shader files are used for both builds.
- `assets/shaders/blinnphong.frag`: 4 small fixes GLSL ES rejects (implicit int→float, e.g. `0` → `0.0`,
  `1 - shadow` → `1.0 - shadow`, `1.0 / textureSize()` → `1.0 / vec2(textureSize())`, and a `const`
  on the Poisson-disk array). They are still valid GLSL 330, same results on desktop.
- `Shader.cpp`: `glDrawBuffer(GL_NONE)` → `glDrawBuffers` on the web; `glFramebufferTexture` is not
  available there.
- `Renderer.cpp`: `GL_MULTISAMPLE` enable/disable skipped on the web (does not exist in WebGL2).
- `SceneManager.cpp`: on the web the GL context attributes (ES 3.0 → WebGL2) are set *before* the context
  is created.

### Game loop
- `main.cpp`: includes `<emscripten.h>` (the existing `emscripten_set_main_loop` call needed it) and stops
  the browser loop once the game is over, leaving the last frame (e.g. "You died") on screen.

### Performance
Target: at least 30 FPS. Measured in Firefox (headless, hardware WebGL on an Intel Iris Plus G7 iGPU,
1280x720, camera idle):

| Configuration                                   | FPS        |
|-------------------------------------------------|------------|
| Original shadow map sizes (2048², 1280x720)      | 27–32      |
| Web build as shipped (shadow maps capped at 1024) | ~53 avg, 47 min |

Changes that only affect the web build:
- **Shadow map size cap** (`WEB_MAX_SHADOW_MAP_SIZE`, default 1024, aspect ratio kept so light projections
  stay the same). Every shadow map (5 spot lights + 6 point light cube faces + flashlight) is re-rendered each
  frame; at 2048² this is what kept the iGPU under 30 FPS. Shadows get slightly softer/blockier.
- **`GLCall` does no error checking in release web builds** (`GlMacros.h`). `glGetError()` is a synchronous
  round trip to the browser's GPU process and was called around every GL call.
- **`PERF` does not print on the web** (`SceneManager.cpp`). It printed 5 lines per frame, each a
  `console.log` in the browser. Instead an averaged `FPS: ...` line is printed every 2 seconds.

### Known limitations on the web
- The page must stay visible: browsers pause `requestAnimationFrame` for hidden tabs, so the game freezes
  (and time jumps forward) while the tab is in the background.
- Audio starts only after the first click/keypress on the page (browser autoplay policy).
- The TAB screenshot writes `screenshot.png` into Emscripten's in-memory file system, so it is not
  downloadable.
