# Dependencies

* cmake
* ninja-build or make
* compiler (see [compilation](Compilation.md) docs)

## Libraries

* development headers/libs for
  * glm
  * lua >= 5.4
  * sdl2 > 2.0.16
  * gtest (and gmock)
  * freetype2 (optional)
  * libjpeg (optional but recommended)
  * libpng (optional)

Some of these dependencies might not be available as packages in your toolchain - most
of them are also bundled with the application. But local installed headers always have
the higher priority. Usually you don't have to install anything of these.

## Debian

```bash
apt-get install binutils-dev libunwind-dev libglm-dev lua5.4 liblua5.4-dev libfreetype-dev libsdl2-dev wayland-protocols pkg-config libjpeg-dev libpng-dev
```

## RockyLinux

```bash
dnf install epel-release
dnf config-manager --set-enabled crb
dnf install SDL2-devel freetype-devel libjpeg-devel binutils-devel make cmake gcc g++ libasan libubsan ninja-build libcurl-devel
```

To also build the rpm files:

```bash
dnf install rpm-devel
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
rpmbuild -ba build/vengi.spec
```

## Arch

```bash
pacman -Sy git make cmake ninja sdl2 clang
```

## Brew

```bash
brew install sdl2 jpeg-turbo freetype cmake ninja pkg-config
```

## Windows

```bash
vcpkg install pkg-config sdl2 libjpeg-turbo lua glm glslang gtest freetype
```
