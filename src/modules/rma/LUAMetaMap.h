/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"
#include "core/collection/DynamicArray.h"

namespace rma {
class MetaMap;
}

extern int luametamap_pushmetamap(lua_State *s, rma::MetaMap *b);
extern void luametamap_setup(lua_State *s, const core::DynamicArray<luaL_Reg> &extensions);
extern rma::MetaMap *luametamap_tometamap(lua_State *s, int idx);
