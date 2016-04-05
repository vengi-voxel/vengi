# About
Voxel engine that depends on [PolyVox](http://www.volumesoffun.com/polyvox-about/).

The build system is a cmake based system that automatically downloads all the needed dependencies. See
[fips homepage](http://floooh.github.io/fips).

Network message generation depends on [flatbuffers](https://github.com/mgerhardy/fips-flatbuffers)

# Var
There are vars inside the engine that you can override via commandline. Var instances are runtime changeable
configuration variables that you can influence from within the game.
e.g. run the server with ```./server -set sv_port 1025``` to change the *sv_port* var to 1025 and bind that port.

# Dependencies
* You need to have **SDL2** installed
 * Windows: env var *SDL2DIR* point to the directory where you extracted it into ([Download link](http://libsdl.org/release/SDL2-devel-2.0.3-VC.zip))
 * Linux: pkg-config support is enough here (e.g. ```apt-get install libsdl2-dev```)
* You need to have **git** installed and in your path
* You need to have **cmake** installed and in your path
* You need to have **python27** installed and in your path
* You need to have **postgre** installed (ubuntu package postgresql, libpq-dev and postgresql-server-dev-9.3 (or another version))

# Compilation
* After fulfilling all the above mentioned dependencies, you just have to run ```fips```

# Profiling
* There is built-in [Remotery](https://github.com/Celtoys/Remotery) support - just run the application with e.g.
    ```./fips run server -- -set core_trace true```
  or without fips:
    ```./server -set core_trace true```
  The *core_trace* var will activate all those core_trace_* macros.

# Check this out:
* [PolyVox and Bullet](http://www.reddit.com/r/VoxelGameDev/comments/2dmfr1/fun_with_polyvox_and_bullet/)
* [PolyVox 3D world generation](http://accidentalnoise.sourceforge.net/minecraftworlds.html)
* [AccidentalNoise islands](http://www.gamedev.net/blog/33/entry-2249282-hooking-into-the-tree-to-build-a-map/)
* [libnoise tutorials](http://libnoise.sourceforge.net/tutorials/)
* [VoxelEditor](https://voxel.codeplex.com/)
* [Voxel Engines](http://www.reddit.com/r/gamedev/wiki/block_engines)
* [Minecraft Protocol](http://wiki.vg/Protocol)
* [CouchDB](http://code.google.com/p/couchdbpp/)
* [Quest Generating AI](http://voxelquest.vanillaforums.com/discussion/comment/11/#Comment_11) (VoxelQuest)
* [VoxelShip Editor](https://blackflux.com/node/11)
* [MagicaVoxel Editor](http://ephtracy.github.io/)
* [The Algorithmic Beauty of Plants](http://algorithmicbotany.org/papers/#abop)
* [Plants vterrain](http://vterrain.org/Plants/)

## PostgreSQL

first sudo as postgres default superuser 'postgres' on bash

`sudo -i -u postgres`

adding an new new user by typing

`createuser -s dbmaster`

create a new database

`createdb engine_db`

now start postgres and add password for these user

`psql`

write this statement

`ALTER USER dbmaster WITH PASSWORD 'ben711cCefIUit887';`

now we have done the first initialization.


### quick postreSQL guide:

to leave PSQL type: `\q`
to list all Database type: `\l`
to connect to an specific database type: `\c dbname` (for example `\c engine_db`)
after you are connected to an database, you can use SQL
to list al tables type: `\dt` (`\d+` for all tables. also sequences)

more stuff: http://wiki.ubuntuusers.de/PostgreSQL

### console postgres tools

temporary tools (till rcon is not final) are usable via console.
after db is initialized start the server and type `store init`
this will create the user table

to add an user type `store useradd <name> <password>`
