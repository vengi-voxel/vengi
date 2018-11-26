/**
 * @file
 */
#pragma once

namespace lua {
class LUA;
}

namespace video {

class Mesh;

extern void meshlua_register(lua::LUA& lua, video::Mesh* mesh);

}
