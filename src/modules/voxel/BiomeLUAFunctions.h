#pragma once

struct lua_State;

namespace voxel {

class Biome;

extern void biomelua_biomeregister(lua_State* s);
extern int biomelua_pushbiome(lua_State* s, Biome* b);

}
