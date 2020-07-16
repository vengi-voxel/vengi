/***
 * @file LUAFunctions.h
 * @ingroup LUA
 */
#pragma once

#include "commonlua/LUA.h"
#include <memory>

namespace backend {

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

extern const char* luaAI_metaregistry();
extern const char *luaAI_metaai();
extern const char* luaAI_metacharacter();

extern void luaAI_registerAll(lua_State* s);
extern int luaAI_pushai(lua_State* s, const AIPtr& ai);

}
