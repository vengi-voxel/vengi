# Dependencies

* cmake
* ninja-build
* postgresql

## Libraries

* development headers/libs for
  * glm
  * lua >= 5.4
  * sdl2 > 2.0.4
  * postgresql-server-dev >= 9.5
  * libpq
  * enet
  * libuv
  * gtest
  * opencl (optional)
  * libuuid
  * mosquitto (optional)

Some of these dependencies might not be available as packages in your toolchain - most
of them are also bundled with the application. But local installed headers always have
the higher priority.

## Debian

```bash
apt-get install libglm-dev lua5.4 liblua5.4-dev libsdl2-dev postgresql-server-dev-all \
    libpq-dev libenet-dev opencl-c-headers \
    wayland-protocols pkg-config uuid-dev libsdl2-mixer-dev libuv1-dev
```

If you want to run the database server locally, you have to install the postgres server package:

```bash
apt-get install postgresql-10 postgresql-contrib
```

```sql
CREATE EXTENSION pgcrypto;
```

## MacPorts

```bash
port install postgresql95-server
```

## Brew

```bash
brew install mosquitto libuv sdl2 libpq sdl2_mixer
```

## Windows

```bash
vcpkg install sdl2 libuv libpq lua glm glslang gtest mosquitto
```
