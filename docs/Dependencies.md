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
  * enet
  * zlib
  * libuv
  * turbobadger
  * gtest
  * opencl
  * libuuid
  * qt (for the rcon tool)

Some of these dependencies might not be available as packages in your toolchain - most of them are also bundled with the application. But local installed headers always have the higher priority.

## Debian
    apt-get install libglm-dev libassimp-dev lua5.3 liblua5.3-dev libsdl2-dev postgresql-server-dev-10 libpq-dev libenet-dev libgtk-3-dev qt5-default qttools5-dev qttools5-dev-tools opencl-c-headers wayland-protocols pkg-config uuid-dev

If you want to run the database server locally, you have to install the postgres server package:

    apt-get install postgresql-10 postgresql-contrib

    CREATE EXTENSION pgcrypto;

## Arch Linux
    pacman [...]

## MacPorts
    port install qt5 postgresql95-server

## Brew
    brew install qt5 mosquitto curl-openssl zlib libuv sdl2 assimp libpq sdl2_mixer

## Windows
    vcpkg install sdl2 curl libuv zlib libpq lua glm glslang gtest qt5
