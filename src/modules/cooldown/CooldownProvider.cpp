/**
 * @file
 */
#include "CooldownProvider.h"

#include "commonlua/LUA.h"
#include "app/App.h"
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
	lua.newGlobalData<CooldownProvider>("Provider", this);

	lua.registerGlobal("addCooldown", [] (lua_State* s) {
		CooldownProvider* data = lua::LUA::globalData<CooldownProvider>(s, "Provider");
		const char *typeStr = luaL_checkstring(s, 1);
		const Type type = getType(typeStr);
		if (type == cooldown::Type::NONE) {
			return lua::LUA::returnError(s, "%s is an invalid cooldown type", typeStr);
		}
		const unsigned long millis = luaL_checkinteger(s, 2);
		Log::debug("set millis for %s to %li", typeStr, millis);
		data->_durations[core::enumVal<Type>(type)] = millis;
		return 0;
	});

	if (!lua.load(cooldowns)) {
		_error = lua.error();
		return false;
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
