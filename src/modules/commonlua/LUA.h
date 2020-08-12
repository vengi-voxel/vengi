/**
 * @file
 */

#pragma once
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "core/collection/DynamicArray.h"
#include "core/String.h"
#include <memory>

#include "core/Log.h"
#include "core/NonCopyable.h"

namespace lua {

const core::String META_PREFIX = "META_";

class LUAType {
private:
	lua_State* _state;
public:
	LUAType(lua_State* state, const core::String& name);

	// only non-capturing lambdas can be converted to function pointers
	template<class FUNC>
	void addFunction(const core::String& name, FUNC&& func) {
		lua_pushcfunction(_state, func);
		lua_setfield(_state, -2, name.c_str());
	}
};

struct LUAFunction {
	const core::String name;
	lua_CFunction func;

	inline LUAFunction(const core::String &_name, lua_CFunction _func) :
			name(_name), func(_func) {
	}
};
using LUAFunctions = core::DynamicArray<lua::LUAFunction>;

class LUA : public core::NonCopyable {
private:
	lua_State *_state;
	core::String _error;
	bool _destroy;
	bool _debug;

	void openState();
	void closeState();

public:
	explicit LUA(lua_State *state);

	explicit LUA(bool debug = false);
	~LUA();

	lua_State* state() const;

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

	template<class T>
	static T** newUserdata(lua_State *L, const core::String& prefix) {
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const core::String name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		return udata;
	}

	template<class T>
	static T* newUserdata(lua_State *L, const core::String& prefix, T* data) {
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const core::String name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		*udata = data;
		return data;
	}

	template<class T>
	static T* userData(lua_State *L, int n, const core::String& prefix) {
		const core::String name = META_PREFIX + prefix;
		return *(T **) luaL_checkudata(L, n, name.c_str());
	}

	/**
	 * Aborts the lua execution with the given error message
	 */
	static int returnError(lua_State *L, const core::String& error) {
		Log::error("LUA error: %s", error.c_str());
		return luaL_error(L, "%s", error.c_str());
	}

	void pop(int amount = 1);

	void reg(const core::String& prefix, const luaL_Reg* funcs);

	void reg(const core::String& prefix, const LUAFunctions& funcs) {
		core::DynamicArray<luaL_Reg> f;
		f.reserve(funcs.size());
		for (const auto& func : funcs) {
			f.emplace_back(luaL_Reg{func.name.c_str(), func.func});
		}
		f.emplace_back(luaL_Reg{nullptr, nullptr});
		reg(prefix, &f[0]);
	}

	LUAType registerType(const core::String& name);

	void setError(const core::String& error);
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
	/**
	 * @brief Executes an 'update' function with a delta time parameter in the lua code
	 */
	bool executeUpdate(uint64_t dt);

	bool valueFloatFromTable(const char* key, float *value);

	int intValue(const core::String& path, int defaultValue = 0);
	float floatValue(const core::String& path, float defaultValue = 0.0f);
	core::String string(const core::String& expr, const core::String& defaultValue = "");

	static core::String stackDump(lua_State *L);
	core::String stackDump();
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

typedef std::shared_ptr<LUA> LUAPtr;

}

#if LUA_VERSION_NUM >= 502
#undef lua_equal
#define lua_equal(L, idx1, idx2) lua_compare(L, (idx1), (idx2), LUA_OPEQ)
#undef lua_objlen
#define lua_objlen lua_rawlen
#endif
