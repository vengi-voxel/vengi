# Server

## General

**Hint**: In order to start the server you will need to setup the postgres database. See
the setup documentation for more details.

After starting the server, you will get entities spawned in the world. You can use e.g. the
ai remote debugger (`src/tools/rcon`) to inspect the state or use the client to connect.

For the client connect you need a user in the database. You can create users with a server
command called `sv_createuser`.

To get a list of available commands, you can use cmdlist in the server terminal.

## Chunk download

The chunks are persisted in the database and can be made available via cdn or any other http
server. The gameserver has a built-in http server, too. If you want to use it, you have to set
the `sv_httpchunkurl` to your gameserver hostname like this:

```
./vengi-server -set sv_httpchunkurl http://myhostname:8080/chunk
```

... or export the environment variable SV_HTTPCHUNKURL or add it to your config file (see
setup documentation for more details).

This cvar is one of those that is automatically replicated to the client. Whenever you change it,
all clients will be notified about it and use the new url.

TODO: document server architecture

## Docker

The docker image is using apt-cacher-ng (https://github.com/sameersbn/docker-apt-cacher-ng)

Make sure to have this running on your docker host

```
docker run --name apt-cacher-ng --init -d --restart=always \
  --publish 3142:3142 \
  --volume /srv/docker/apt-cacher-ng:/var/cache/apt-cacher-ng \
  sameersbn/apt-cacher-ng:3.1-1
```
