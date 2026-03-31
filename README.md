# simple-horror-game

This is a simple horror game implemented in OpenGL inspired by the classic Slenderman game. 
It was created as an assignment project for the graphics course at the [CS Master's of Athens University of Economics](https://grad.cs.aueb.gr) and business with [FotiosBistas](https://github.com/fotiosbistas) who implemented most of the core graphics algorithms and the related shaders, while I made the game logic, added sound,an NPC, text rendering and event handling. 

[Simple-Horror-Game-Short-Video.webm](https://github.com/user-attachments/assets/10b4337b-e2d7-4579-abf3-68d678d105b4)

It is implemented using the following libraries(and CMake for build system): 

- **GLM** for math operations
- **SDL2** for windowing and input operations
- **libtiff** for .tiff files (for textures)
- **freetype** for loading .ttf fonts
- **GLEW** for opengl related operations
- **stb_image** for loading textures

Some of the features implemented:
- Shadows and Lightning system
- .obj files parsing
- NPC that toggles between chasing the player and roaming(with a programmable interface for custom scripts)
- Collisions w. AABB (for the monster they can be toggled on/off for more difficulty)

Things practiced:

- CMake Usage(extracting libraries mostly to implement proper architecture and be able to change graphics backends)
- Architecture through abstractions and refactoring
- Codegen using CMake for build-time parsing of model files(a failed experiment as a 20Meg cpp file cant get parsed seems to be the reason of failure)
- Github actions(and using vcpkg for cross-platform builds)
- Using clang tooling to implement asset tree shaking (not implemented in CI yet)

