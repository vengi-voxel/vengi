/**
 * @file
 */
#include "CooldownProvider.h"

#include "commonlua/LUA.h"
#include "core/App.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/io/Filesystem.h"

namespace cooldown {

CooldownProvider::CooldownProvider() {
	for (int i = 0; i < lengthof(_durations); ++i) {
		_durations[i] = DefaultDuration;
	}
}

long CooldownProvider::setDuration(Type type, long duration) {
	const int32_t t = std::enum_value<Type>(type);
	const long old = _durations[t];
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
			luaL_error(s, "%s is an invalid cooldown type", typeStr);
		}
		const long millis = luaL_checkinteger(s, 2);
		Log::debug("set millis for %s to %li", typeStr, millis);
		data->_durations[std::enum_value<Type>(type)] = millis;
		return 0;
	});

	if (!lua.load(cooldowns)) {
		_error = lua.error();
		return false;
	}

	_initialized = true;
	return true;
}

}
