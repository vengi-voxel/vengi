/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"

namespace attrib {

class ContainerProvider;

extern void luaattrib_setup(lua_State* s, ContainerProvider* provider);

}
