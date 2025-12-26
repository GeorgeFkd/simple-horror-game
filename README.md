# simple-horror-game

This is a simple horror game implemented in OpenGL inspired by the classic Slenderman game. 
It was created as an assignment project for the graphics course at the CS Master's of Athens University of Economics and business with [FotiosBistas](https://github.com/fotiosbistas) who implemented most of the core graphics algorithms and the related shaders, while I made the game logic, added sound,an NPC, text rendering and event handling. 

[Simple-Horror-Game-Short-Video.webm](https://github.com/user-attachments/assets/10b4337b-e2d7-4579-abf3-68d678d105b4)

It is implemented using the following libraries(and CMake for build system): 

- **GLM** for math operations
- **SDL2** for windowing and input operations
- **libtiff** for .tiff files
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

