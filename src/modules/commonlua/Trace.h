/**
 * @file
 */

#pragma once

#ifdef TRACY_ENABLE
#include "core/tracy/public/tracy/TracyLua.hpp"
#endif

namespace lua {

#ifndef TRACY_ENABLE
static int trace_Dummy(lua_State* s) {
	return 0;
}
#endif

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
	lua_setglobal(s, "tracy");
	return true;
#endif
}

}
