# Server

## General

**Hint**: In order to start the server you will need to setup the postgres database. See
the setup documentation for more details.

After starting the server, you will get entities spawned in the world. You can use e.g. the
ai remote debugger (`src/tools/rcon`) to inspect the state or use the client to connect.

For the client connect you need a user in the database. You can create users with a server
command called `sv_createuser`.

To get a list of available commands, you can use cmdlist in the server terminal.

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
