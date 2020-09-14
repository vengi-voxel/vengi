/**
 * @file
 */

#pragma once

#include "core/Log.h"

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
	void setError(const core::String& error);

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

	template<class T>
	static T* newGlobalData(lua_State *L, const core::String& prefix, T *userData) {
		lua_pushlightuserdata(L, userData);
		lua_setglobal(L, prefix.c_str());
		return userData;
	}

	template<class T>
	inline T* newGlobalData(const core::String& prefix, T *userData) const {
		newGlobalData(_state, prefix, userData);
		return userData;
	}

	template<class T>
	static T* globalData(lua_State *L, const core::String& prefix) {
		lua_getglobal(L, prefix.c_str());
		T* data = (T*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return data;
	}

	template<class T>
	inline T* globalData(const core::String& prefix) const {
		return globalData<T>(_state, prefix);
	}

	template<class FUNC>
	inline void registerGlobal(const char* name, FUNC&& f) const {
		lua_pushcfunction(_state, f);
		lua_setglobal(_state, name);
	}

	void pop(int amount = 1);

	void reg(const core::String& prefix, const luaL_Reg* funcs);

	const core::String& error() const;
	/**
	 * @brief Loads a lua script into the lua state.
	 */
	bool load(const core::String &luaString);
	/**
	 * @brief Executes a function from an already loaded lua state
	 * @param[in] function function to be called
	 * @param[in] returnValues The amount of values returned by the called lua function. -1 is for multiple return values.
	 * @note Use clua_get<T>(s, -1) to get the first custom return value.
	 */
	bool execute(const core::String &function, int returnValues = 0);

	bool valueFloatFromTable(const char* key, float *value);

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

}

#if LUA_VERSION_NUM >= 502
#undef lua_equal
#define lua_equal(L, idx1, idx2) lua_compare(L, (idx1), (idx2), LUA_OPEQ)
#undef lua_objlen
#define lua_objlen lua_rawlen
#endif
