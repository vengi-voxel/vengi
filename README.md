# About
Voxel engine with procedural generated landscape.

![Screenshot](/screenshots/2016-05-05.png "Status")

# Dependencies
* **cmake**
* **postgre**
* development headers/libs for
** glm
** assimp
** lua >= 5.3
** sdl2 > 2.0.4
** postgresql-server-dev >= 9.5
** libpq

## Debian
apt-get install libglm-dev libassimp-dev lua5.3 liblua5.3-dev libsdl2-dev postgresql-server-dev-9.5 libpq-dev

# Var
There are vars inside the engine that you can override via commandline. Var instances are runtime changeable
configuration variables that you can influence from within the game.
e.g. run the server with ```./server -set sv_port 1025``` to change the *sv_port* var to 1025 and bind that port.

# Profiling
* There is built-in [Remotery](https://github.com/Celtoys/Remotery) support - just run the application with e.g.
    ```./server -set core_trace true```
  The *core_trace* var will activate all those core_trace_* macros.
