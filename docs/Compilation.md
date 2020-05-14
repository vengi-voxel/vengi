# Building {#building}

## Linux

You can just run `make` in the project root folder.

Every project has some extra CMake targets. There are e.g. `voxedit-run`, `voxedit-debug` and `voxedit-perf` if the needed tools were found during cmake's configure phase.

That means that you can compile a single target by typing `make voxedit`, run it by typing `make voxedit-run`, debug it by typing `make voxedit-debug` and profile it by
typing `make voxedit-perf`. There are also other targets for valgrind - just use the tab compiltion in the build folder to get a list.

## Windows

The project should be buildable with every ide that supports cmake. QTCreator, Eclipse CDT, vscode or Visual Studio.

## Mac

You can generate your xcode project via cmake.
