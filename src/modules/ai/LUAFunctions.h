/***
 * @file LUAFunctions.h
 * @ingroup LUA
 */
#pragma once

#include "commonlua/LUA.h"
#include <memory>

namespace ai {

class AI;
typedef std::shared_ptr<AI> AIPtr;

template<class T>
static T* luaAI_getlightuserdata(lua_State *s, const char *name) {
	lua_getglobal(s, name);
	if (lua_isnil(s, -1)) {
		return nullptr;
	}
	T* data = (T*) lua_touserdata(s, -1);
	lua_pop(s, 1);
	return data;
}

template<class T>
static inline T luaAI_getudata(lua_State* s, int n, const char *name) {
	void* dataVoid = luaL_checkudata(s, n, name);
	if (dataVoid == nullptr) {
		core::String error(name);
		error.append(" userdata must not be null");
		luaL_argcheck(s, dataVoid != nullptr, n, error.c_str());
	}
	return (T) dataVoid;
}

template<class T>
static inline T* luaAI_newuserdata(lua_State* s, const T& data) {
	T* udata = (T*) lua_newuserdata(s, sizeof(T));
	*udata = data;
	return udata;
}

static inline int luaAI_assignmetatable(lua_State* s, const char *name) {
	luaL_getmetatable(s, name);
#if AI_LUA_SANTITY
	if (!lua_istable(s, -1)) {
		ai_log_error("LUA: metatable for %s doesn't exist", name);
		return 0;
	}
#endif
	lua_setmetatable(s, -2);
	return 1;
}

template<class T>
static inline int luaAI_pushudata(lua_State* s, const T& data, const char *name) {
	luaAI_newuserdata<T>(s, data);
	return luaAI_assignmetatable(s, name);
}

extern const char* luaAI_metaregistry();
extern const char *luaAI_metaai();
extern const char* luaAI_metacharacter();

extern void luaAI_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name);
extern void luaAI_registerAll(lua_State* s);
extern int luaAI_pushai(lua_State* s, const AIPtr& ai);

}
