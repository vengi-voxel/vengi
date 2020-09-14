/**
 * @file
 */
#include "CooldownProvider.h"

#include "commonlua/LUA.h"
#include "app/App.h"
#include "commonlua/LUAFunctions.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "io/Filesystem.h"
#include "core/Log.h"

namespace cooldown {

CooldownProvider::CooldownProvider() {
	for (int i = 0; i < lengthof(_durations); ++i) {
		_durations[i] = DefaultDuration;
	}
}

unsigned long CooldownProvider::setDuration(Type type, unsigned long duration) {
	const int32_t t = core::enumVal<Type>(type);
	const unsigned long old = _durations[t];
	_durations[t] = duration;
	return old;
}

bool CooldownProvider::init(const core::String& cooldowns) {
	if (cooldowns.empty()) {
		_error = "";
		_initialized = true;
		return true;
	}

	if (cooldowns.empty()) {
		_error = "Could not load lua script";
		return false;
	}

	_error = "";

	lua::LUA lua;

	if (!lua.load(cooldowns, 1)) {
		_error = lua.error();
		return false;
	}

	if (!lua_istable(lua, -1)) {
		Log::error("Expected a table with cooldown data");
		return false;
	}

	lua_pushnil(lua);					// push nil, so lua_next removes it from stack and puts (k, v) on stack
	while (lua_next(lua, -2) != 0) {	// -2, because we have table at -1
		if (!lua_isinteger(lua, -1) || !lua_isstring(lua, -2)) {
			Log::error("Expected to find string as key and integer as value");
			return false;
		}
		const char *key = lua_tostring(lua, -2);
		const int value = lua_tointeger(lua, -1);
		const Type type = getType(key);
		if (type == cooldown::Type::NONE) {
			Log::error("%s is an invalid cooldown type", key);
			continue;
		}
		Log::debug("set millis for %s to %i", key, value);
		_durations[core::enumVal<Type>(type)] = value;
		lua_pop(lua, 1); // remove value, keep key for lua_next
	}

	_initialized = true;
	return true;
}

unsigned long CooldownProvider::duration(Type type) const {
	if (!_initialized) {
		::Log::warn("Trying to get cooldown duration without CooldownProvider::init() being called");
	}
	return _durations[core::enumVal<Type>(type)];
}

}
