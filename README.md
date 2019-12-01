[![Actions Status](https://github.com/mgerhardy/engine/workflows/build/badge.svg)](https://github.com/mgerhardy/engine/actions)
 [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# About
Voxel engine and tools.

# Contact

- [Discord Server](https://discord.gg/AgjCPXy)

# Tools
* [The voxel editor](src/tools/voxedit/README.md)
* [World viewer](src/tools/mapview/README.md)
* [Remote AI Debugger](src/tools/rcon/README.md)
* [Database tool](src/tools/databasetool/README.md)
* [Shader tool](src/tools/shadertool/README.md)
* [Compute Shader tool](src/tools/computeshadertool/README.md)
* [Traze client](src/tests/testtraze/README.md)

# General
* [Dependencies](docs/Dependencies.md)
* [Compilation](docs/Compilation.md)
* [Setup](docs/Setup.md)
* [GameDesign](docs/GameDesign.md)

# Running the tests
The tests need a postgres database named `engine` and a user named `engine` with the password.... `engine` (you can modify them via cvars). If no OpenGL or OpenCL context is available, the related tests are skipped.
You can use the docker-compose files to set up your environment.

# More information
For more information, please check out the [wiki](https://gitlab.com/mgerhardy/engine/wikis/home) or the [doxygen](https://mgerhardy.gitlab.io/engine/) documentation.

You can find some old and most likely outdated videos in my [youtube channel](https://www.youtube.com/channel/UCbnJUW0d4tYvdmsJ-R6iUpA).
