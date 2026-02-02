/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/Log.h"
#include "engine-config.h"

extern "C" {

#define lua_writestringerror c_logerror
#define lua_writestring c_logwrite
#define lua_writeline() c_logwrite("\n", 1)

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "core/String.h"
#include "core/NonCopyable.h"

// https://wiki.gentoo.org/wiki/Lua/Porting_notes

namespace lua {

const core::String META_PREFIX = "META_";

class LUA : public core::NonCopyable {
private:
	lua_State *_state;
	core::String _error;
	bool _destroy;
	bool _debug;

	void openState();
	void closeState();
	core::String string(const core::String& expr, const core::String& defaultValue = "");

public:
	explicit LUA(lua_State *state);

	explicit LUA(bool debug = false);
	~LUA();

	lua_State* state() const;
	inline operator lua_State*() const {
		return state();
	}

	/**
	 * @return @c false if the state is not managed by this instance
	 */
	bool resetState();

	void pop(int amount = 1);

	void setError(const core::String& error);
	const core::String& error() const;
	/**
	 * @brief Loads a lua script into the lua state.
	 */
	bool load(const core::String &luaString, int returnValues = 0);
	/**
	 * @brief Executes a function from an already loaded lua state
	 * @param[in] function function to be called
	 * @param[in] returnValues The amount of values returned by the called lua function. -1 is for multiple return values.
	 * @note Use clua_get<T>(s, -1) to get the first custom return value.
	 */
	bool execute(const core::String &function, int returnValues = 0);

	int intValue(const core::String& path, int defaultValue = 0);
	float floatValue(const core::String& path, float defaultValue = 0.0f);
};

inline lua_State* LUA::state() const {
	return _state;
}

inline void LUA::setError(const core::String& error) {
	_error = error;
}

inline const core::String& LUA::error() const {
	return _error;
}

class StackChecker {
private:
	lua_State *_state;
	const int _startStackDepth;
public:
	StackChecker(lua_State *state) :
			_state(state), _startStackDepth(lua_gettop(_state)) {
	}
	~StackChecker() {
		core_assert_msg(_startStackDepth == lua_gettop(_state), "Lua stack imbalance: started with %i, ended with %i",
						_startStackDepth, lua_gettop(_state));
	}
};

}

#if LUA_VERSION_NUM >= 502
#undef lua_equal
#define lua_equal(L, idx1, idx2) lua_compare(L, (idx1), (idx2), LUA_OPEQ)
#undef lua_objlen
#define lua_objlen lua_rawlen
#endif
