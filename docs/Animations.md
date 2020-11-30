# Animations

There is a [test application](TestAnimation.md) that is able to play all of the supported animations and to test them during development..

## LUA

You can write your animations in lua scripts - but keep in mind that this is of course not as fast as writing them in C++ - you should only use this during development for faster round trip times.

Besides writing the animations in lua, the skeleton properties are stored in lua.

## Hot reloading

*(This is for the C++ code)*

For debug builds, the animations are linked into a shared object that is loaded into the `AnimationSystem` class. This feature is __not__ active for release builds.

If you would like to disable the hot reload support - call cmake with `-DHOT_RELOAD_ANIM=0`.

Besides the dynamic loading of the animation functions, we also install a watch on the dynamic library. Whenever the library is recompiled and relinked, the applications will automatically reload them and present you the latest code changes.

The cvar that stores the value of the path to the library is called `anim_lib`. It should get filled automatically. But if not, either export the environment variable `ANIM_LIB` to the full path of that lib, or set the cvar `anim_lib` to the full path to the lib.
