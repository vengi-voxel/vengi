#include "LUAFunctions.h"

int clua_assignmetatable(lua_State* s, const char *name) {
	luaL_getmetatable(s, name);
	if (!lua_istable(s, -1)) {
		Log::error("LUA: metatable for %s doesn't exist", name);
		return 0;
	}
	lua_setmetatable(s, -2);
	return 1;
}

void clua_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name) {
	luaL_newmetatable(s, name);
	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	luaL_setfuncs(s, funcs, 0);
}
