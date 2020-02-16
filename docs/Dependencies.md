# Dependencies
* cmake
* ninja-build
* postgre
* development headers/libs for
  * glm
  * lua >= 5.3
  * sdl2 > 2.0.4
  * postgresql-server-dev >= 9.5
  * libpq
  * enet
  * libuv
  * turbobadger
  * gtest
  * opencl
  * libuuid
  * qt (for the rcon tool)
  * mosquitto (optional)

Some of these dependencies might not be available as packages in your toolchain - most
of them are also bundled with the application. But local installed headers always have
the higher priority.

## Debian
    apt-get install libglm-dev lua5.3 liblua5.3-dev libsdl2-dev postgresql-server-dev-all \
      libpq-dev libenet-dev qt5-default qttools5-dev qttools5-dev-tools opencl-c-headers \
      wayland-protocols pkg-config uuid-dev libsdl2-mixer-dev

If you want to run the database server locally, you have to install the postgres server package:

    apt-get install postgresql-10 postgresql-contrib

    CREATE EXTENSION pgcrypto;

## Arch Linux
    pacman [...]

## MacPorts
    port install qt5 postgresql95-server

## Brew
    brew install qt5 mosquitto libuv sdl2 libpq sdl2_mixer

## Windows
    vcpkg install sdl2 libuv libpq lua glm glslang gtest qt5 mosquitto
