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
