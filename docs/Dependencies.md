# Dependencies

* cmake
* ninja-build or make
* compiler (see [compilation](Compilation.md) docs)

## Libraries

* development headers/libs for
  * glm
  * lua >= 5.4
  * sdl2 > 2.0.16
  * libuv
  * gtest (and gmock)
  * opencl (optional)
  * libuuid

Some of these dependencies might not be available as packages in your toolchain - most
of them are also bundled with the application. But local installed headers always have
the higher priority. Usually you don't have to install anything of these.

## Debian

```bash
apt-get install libglm-dev lua5.4 liblua5.4-dev libsdl2-dev opencl-c-headers \
    wayland-protocols pkg-config uuid-dev libuv1-dev
```

## Arch

```bash
pacman -Sy git make cmake ninja libuv sdl2 clang
```

## Brew

```bash
brew install libuv sdl2
```

## Windows

```bash
vcpkg install sdl2 libuv lua glm glslang gtest
```
