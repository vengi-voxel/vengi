/**
 * @file
 */

#include "LUACondition.h"
#include "backend/entity/ai/LUAFunctions.h"

namespace backend {

bool LUACondition::evaluateLUA(const AIPtr& entity) {
	// get userdata of the condition
	const core::String name = "__meta_condition_" + _name;
	lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());
#if AI_LUA_SANTITY > 0
	if (lua_isnil(_s, -1)) {
		Log::error("LUA condition: could not find lua userdata for %s", _name.c_str());
		return false;
	}
#endif
	// get metatable
	lua_getmetatable(_s, -1);
#if AI_LUA_SANTITY > 0
	if (!lua_istable(_s, -1)) {
		Log::error("LUA condition: userdata for %s doesn't have a metatable assigned", _name.c_str());
		return false;
	}
#endif
	// get evaluate() method
	lua_getfield(_s, -1, "evaluate");
	if (!lua_isfunction(_s, -1)) {
		Log::error("LUA condition: metatable for %s doesn't have the evaluate() function assigned", _name.c_str());
		return false;
	}

	// push self onto the stack
	lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());

	// first parameter is ai
	if (luaAI_pushai(_s, entity) == 0) {
		return false;
	}

#if AI_LUA_SANTITY > 0
	if (!lua_isfunction(_s, -3)) {
		Log::error("LUA condition: expected to find a function on stack -3");
		return false;
	}
	if (!lua_isuserdata(_s, -2)) {
		Log::error("LUA condition: expected to find the userdata on -2");
		return false;
	}
	if (!lua_isuserdata(_s, -1)) {
		Log::error("LUA condition: second parameter should be the ai");
		return false;
	}
#endif
	const int error = lua_pcall(_s, 2, 1, 0);
	if (error) {
		Log::error("LUA condition script: %s", lua_isstring(_s, -1) ? lua_tostring(_s, -1) : "Unknown Error");
		// reset stack
		lua_pop(_s, lua_gettop(_s));
		return false;
	}
	const int state = lua_toboolean(_s, -1);
	if (state != 0 && state != 1) {
		Log::error("LUA condition: illegal evaluate() value returned: %i", state);
		return false;
	}

	// reset stack
	lua_pop(_s, lua_gettop(_s));
	return state == 1;
}

}
