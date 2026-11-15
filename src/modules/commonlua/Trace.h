/**
 * @file
 */

#pragma once

#include <lua.h>

#ifdef TRACY_ENABLE
// TracyLua.hpp must be included after the Lua headers (see Tracy manual).
#include "core/tracy/public/tracy/TracyLua.hpp"
#endif

namespace lua {

#ifndef TRACY_ENABLE
static int trace_Dummy(lua_State* s) {
	return 0;
}
#endif

/**
 * Register the global @c tracy table for manual Lua instrumentation
 * (@c ZoneBegin / @c ZoneEnd / …) as described in the Tracy manual.
 */
static bool clua_registertrace(lua_State* s) {
#ifdef TRACY_ENABLE
	tracy::LuaRegister(s);
	return true;
#else
	lua_newtable(s);
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneBegin");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneBeginN");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneBeginS");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneBeginNS");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneEnd");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneText");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "ZoneName");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "Message");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "SectionEnter");
	lua_pushcfunction(s, trace_Dummy);
	lua_setfield(s, -2, "SectionLeave");
	lua_setglobal(s, "tracy");
	return true;
#endif
}

}
