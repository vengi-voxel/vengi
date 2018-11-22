# General

TODO: document server architecture

# Docker

The docker image is using apt-cacher-ng (https://github.com/sameersbn/docker-apt-cacher-ng)

Make sure to have this running on your docker host

```
docker run --name apt-cacher-ng --init -d --restart=always \
  --publish 3142:3142 \
  --volume /srv/docker/apt-cacher-ng:/var/cache/apt-cacher-ng \
  sameersbn/apt-cacher-ng:3.1-1
```
