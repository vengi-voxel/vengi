/**
 * @file
 */

#pragma once

#ifdef TRACY_ENABLE
#include "core/tracy/TracyLua.hpp"
#endif

namespace lua {

static bool clua_registertrace(lua_State* s) {
#ifdef TRACY_ENABLE
	tracy::LuaRegister(s);
	return true;
#else
	return false;
#endif
}

}
