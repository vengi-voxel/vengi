[![Build status](https://ci.appveyor.com/api/projects/status/556vyuwwg476jn7t?svg=true)](https://ci.appveyor.com/project/mgerhardy/engine)
[![Build Status](https://travis-ci.org/mgerhardy/engine.svg?branch=master)](https://travis-ci.org/mgerhardy/engine)

# About
Voxel engine with procedural generated landscape.

![Screenshot](/screenshots/2016-12-14.png "Status")
![Screenshot](/screenshots/2017-01-27-voxedit.png "Voxel editor")
![Screenshot](/screenshots/2017-03-01-noisetool.png "Noise tool")

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
 * nativefiledialog (+gtk3 or qt on linux)
 * enet
 * zlib
 * turbobadger
 * sauce/fruit
 * gtest
 * qt (for the rcon tool)

Some of these dependencies might not be available as packages in your toolchain - most of them are also bundled with the application. But local installed headers always have the higher priority.

## Debian
    apt-get install libglm-dev libassimp-dev lua5.3 liblua5.3-dev libsdl2-dev postgresql-server-dev-9.5 libpq-dev libenet-dev libgtk-3-dev qt5-default qttools5-dev qttools5-dev-tools

## Arch Linux
    pacman [...]

## MacPorts
    port install qt5 postgresql95-server

# Building
You can just run ```make``` in the project root folder.

# More information
For more information, please check out the [wiki](https://gitlab.com/mgerhardy/engine/wikis/home) or the [doxygen](https://mgerhardy.gitlab.io/engine/) documentation.
