# About
Voxel engine that depends on [PolyVox](http://www.volumesoffun.com/polyvox-about/) and [AccidentalNoise](http://accidentalnoise.sourceforge.net/)

The build system is a cmake based system that automatically downloads all the needed dependencies. See
[fips homepage](http://floooh.github.io/fips).

Network message generation depends on [flatbuffers](https://github.com/mgerhardy/fips-flatbuffers)

# Var
There are vars inside the engine that you can override via commandline. Var instances are runtime changeable
configuration variables that you can influence from within the game.
e.g. run the server with ```./server -set sv_port 1025``` to change the *sv_port* var to 1025 and bind that port.

* cl_cammaxpitch the max pitch should not be bigger than 89.9 - because at 90 we have a visual switch
* cl_camspeed
* cl_cammousespeed
* sv_seed the server side seed
* cl_name the name of the client
* cl_port the port to connect to
* cl_host e.g. 127.0.0.1
* cl_fullscreen true|false
* cl_chunksize 16 The size of the chunk that is extracted with each step
* core_loglevel 0 = Error => 4 = Trace
* cl_vsync false|true To disable|enable vsync

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



# AI
* Request
* Produce
* Consume

## Production chain
Define a need, to be able to produce something
In order to fulfill this need, the ai should request the needed objects (or walk to a location to get it)
Even food can be handled via "request -> produce" (the production is the health)

We need a system that is able to define the needs and define how to fulfill them.
* Points of Interest (where is something produced)
* Reference other production chains (The product of something else might be the 'request' to 'produce' my own product
* Price one has to pay (exchange with some other good [e.g. of of the 'requests' of the producer] or money)

So every npc should have several requests:
* money (used to get the production chain start good)
* food (used to produce health)
* production chain start good (use to produce something to get money/food/production chain start good)

Each of these requests will get a priority. This way we have unique characters where one is just trying to survive, while another one
is just trying to get rich and yet another one that is already satisfied if he can fulfill the need of others. (The food should always be relative high)

## Requests
requests[] = {
	health
	money
}

## Health
Possibe production chains:
Money -> Food -> Health
InputGood -> OutputGood -> $ReferenceToFoodCreatorRequests

So we need to save the needed outputs - and then recursively search for stuff that leads to fulfilling the need. If we can
fulfill the need on the first level, skip everything else - otherwise go a level deeper.

fulfills[health] = { food 100% }
fulfills[food] = { buyfood 80%, createfood 15%, stealfood 5% }
fulfills[createfood] = { huntanimals 50%, producefood 50% }
fulfills[buyfood] = { money 100% }
fulfills[money] = { sellproduction 95%, stealmoney 5% }
fulfills[sellproduction] = { produce 100% }
fulfills[produce] = { work 99%, findprofession 1% }
fulfills[work] = { getmaterial 1%, convertmaterial 99% }
[...]

I think this can all be modeled into behaviour trees. We need to implement the slot mechanism into SimpleAI to replace
profession slot. So the behaviour tree would e.g. look like this:



                                    CreateFood[IfFoodCreator]
                                    BuyFood[IfEoughMoney]
                 GetFood[IsHungry]  StealFood
    StayHealthy
                 Work


## Requests can't be fulfilled
* Walk to the town hall and cry about not being able to fulfill your need. Do that for x time units - after that change your
profession, get new requests and try to fulfill those.
* A player can now walk into the town hall to resolve unfulfilled requests
* After x time units the unfulfilled request is again visible in the town hall for other players.

# Spawn of npcs
* We need points of interest and a fillrate at those POIs.
* We need to be able to pick a profession (visual appearance will be tight to the profession)
* Do we need a sheriff in each town? One that already investigates unfulfilled requests?
* We need one town master chief rocker in the town hall that gives away the collected quests to the users.
