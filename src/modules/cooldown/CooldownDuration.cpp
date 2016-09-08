#include "CooldownDuration.h"
#include "commonlua/LUA.h"
#include "core/App.h"
#include <SDL.h>

namespace cooldown {

CooldownDuration::CooldownDuration() {
	for (size_t i = 0u; i < SDL_arraysize(_durations); ++i) {
		_durations[i] = DefaultDuration;
	}
}

bool CooldownDuration::init(const std::string& filename) {
	const std::string& cooldowns = core::App::getInstance()->filesystem()->load(filename);
	if (cooldowns.empty()) {
		_error = "Could not load file " + filename;
		return false;
	}

	_error = "";

	lua::LUA lua;
	lua.newGlobalData<CooldownDuration>("CooldownDuration", this);

	lua.registerGlobal("addCooldown", [] (lua_State* s) {
		lua_getglobal(s, "CooldownDuration");
		CooldownDuration* data = (CooldownDuration*) lua_touserdata(s, -1);
		lua_pop(s, 1);
		const char *typeStr = luaL_checkstring(s, 1);
		const Type type = getType(typeStr);
		if (type == cooldown::Type::NONE) {
			luaL_error(s, "%s is an invalid cooldown type", typeStr);
		}
		const long millis = luaL_checkinteger(s, 2);
		Log::info("set millis for %s to %li", typeStr, millis);
		data->_durations[core::enumValue<Type>(type)] = millis;
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
