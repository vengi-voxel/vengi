[![Build status](https://ci.appveyor.com/api/projects/status/556vyuwwg476jn7t?svg=true)](https://ci.appveyor.com/project/mgerhardy/engine)
[![Build Status](https://travis-ci.org/mgerhardy/engine.svg?branch=master)](https://travis-ci.org/mgerhardy/engine)

# About
Voxel engine and tools.

# Tools
* [The voxel editor](src/tools/voxedit/README.md)
* [World viewer](src/tools/mapedit/README.md)
* [Remote AI Debugger](src/tools/rcon/README.md)
* [Database tool](src/tools/databasetool/README.md)
* [Shader tool](src/tools/shadertool/README.md)
* [Compute Shader tool](src/tools/computeshadertool/README.md)
* [Database tool](src/tools/databasetool/README.md)
* [Traze client](src/tests/testtraze/README.md)

# General
* [Dependencies](docs/Dependencies.md)
* [Compilation](docs/Compilation.md)
* [Setup](docs/Setup.md)

# Running the tests
The tests need a postgres database named `engine` and a user named `engine` with the password.... `engine` (you can modify them via cvars). If no OpenGL or OpenCL context is available, the related tests are skipped.

# More information
For more information, please check out the [wiki](https://gitlab.com/mgerhardy/engine/wikis/home) or the [doxygen](https://mgerhardy.gitlab.io/engine/) documentation.

You can find some old and most likely outdated videos in my [youtube channel](https://www.youtube.com/channel/UCbnJUW0d4tYvdmsJ-R6iUpA).
