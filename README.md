[![Build Status](https://gitlab.com/mgerhardy/engine/badges/master/build.svg)](https://gitlab.com/mgerhardy/engine/commits/master)

# About
Voxel engine with procedural generated landscape.

![Screenshot](/screenshots/2016-07-11.png "Status")

# Dependencies
* cmake
* postgre
* development headers/libs for
 * glm
 * assimp
 * lua >= 5.3
 * sdl2 > 2.0.4
 * libcurl
 * postgresql-server-dev >= 9.5
 * libpq
 * flatbuffers
 * enet
 * zlib
 * turbobadger
 * sauce/fruit
 * gtest
 * qt (for the rcon tool)

Some of these dependencies might not be available as packages in your toolchain - most of them are also bundled with the application. But local installed headers always have the higher priority.

## Debian
    apt-get install libglm-dev libassimp-dev lua5.3 liblua5.3-dev libsdl2-dev postgresql-server-dev-9.5 libpq-dev libenet-dev

## Arch Linux
    pacman [...]

## MacPorts
    port install qt5 postgresql95-server

# Building
You can just run ```make``` in the project root folder.

# More information
For more information, please check out the [wiki](https://gitlab.com/mgerhardy/engine/wikis/home) or the [doxygen](https://mgerhardy.gitlab.io/engine/) documentation.
