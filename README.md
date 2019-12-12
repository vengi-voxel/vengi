[![Actions Status](https://github.com/mgerhardy/engine/workflows/build/badge.svg)](https://github.com/mgerhardy/engine/actions)
 [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# About
Voxel engine for mmorpg game and tools.

Includes behaviour tree based ai, a remote ai debugger, a voxel editor and a lot more. The scripting is lua based.

There is still a lot of stuff missing where we could need your help.

You can find some old and most likely outdated videos in my [youtube channel](https://www.youtube.com/channel/UCbnJUW0d4tYvdmsJ-R6iUpA).

Join our [discord Server](https://discord.gg/AgjCPXy) and help developing the game.

# Tools
* [The voxel editor](src/tools/voxedit/README.md)
* [World viewer](src/tools/mapview/README.md)
* [Remote AI Debugger](src/tools/rcon/README.md)
* [Database tool](src/tools/databasetool/README.md)
* [Shader tool](src/tools/shadertool/README.md)
* [Compute Shader tool](src/tools/computeshadertool/README.md)
* [Visual test applications](src/tests/README.md)

# General
* [Dependencies](docs/Dependencies.md)
* [Compilation](docs/Compilation.md)
* [Setup](docs/Setup.md)
* [Configuration](docs/Configuration.md)
* [GameDesign](docs/GameDesign.md)

# Running the tests
The tests need a postgres database named `engine` and a user named `engine` with the password.... `engine` (you can modify them via cvars). If no OpenGL or OpenCL context is available, the related tests are skipped.
You can use the docker-compose files to set up your environment.
