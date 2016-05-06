/**
 * @file
 */

#pragma once
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <map>
#include <iostream>
#include <memory>

namespace lua {

namespace {
const std::string META_PREFIX = "META_";
}

class LUAType {
private:
	lua_State* _state;
public:
	LUAType(lua_State* state, const std::string& name);
	virtual ~LUAType() {
	}

	void addFunction(const std::string& name, lua_CFunction func);
};

class LUA {
private:
	lua_State *_state;
	std::string _error;

public:
	LUA(bool debug = false);
	~LUA();

	inline lua_State* getState() const;

	template<class T>
	static void newGlobalData(lua_State *L, const std::string& prefix, T *userData) {
		lua_pushlightuserdata(L, userData);
		lua_setglobal(L, prefix.c_str());
	}

	template<class T>
	inline void newGlobalData(const std::string& prefix, T *userData) const {
		newGlobalData(_state, prefix, userData);
	}

	template<class T>
	static T* getGlobalData(lua_State *L, const std::string& prefix) {
		lua_getglobal(L, prefix.c_str());
		T* data = (T*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return data;
	}

	template<class T>
	inline T* getGlobalData(const std::string& prefix) const {
		return getGlobalData<T>(_state, prefix);
	}

	template<class T>
	static T** newUserdata(lua_State *L, const std::string& prefix) {
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const std::string name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		return udata;
	}

	template<class T>
	static T* getUserData(lua_State *L, int n, const std::string& prefix) {
		const std::string name = META_PREFIX + prefix;
		return *(T **) luaL_checkudata(L, n, name.c_str());
	}

	static void returnError(lua_State *L, const std::string& error) {
		std::cerr << "LUA error: " << error << std::endl;
		luaL_error(L, "%s", error.c_str());
	}

	void getGlobal(const std::string& name);

	std::string getKey();

	void getGlobalKeyValue(const std::string& name);

	bool getNextKeyValue();

	void pop(int amount = 1);

	int getTable(const std::string& name);

	std::string getTableString(int i);

	int getTableInteger(int i);

	float getTableFloat(int i);

	void reg(const std::string& prefix, luaL_Reg* funcs);
	LUAType registerType(const std::string& name);

	void setError(const std::string& error);
	const std::string& getError() const;
	bool load(const std::string &file);
	/**
	 * @param[in] function function to be called
	 */
	bool execute(const std::string &function, int returnValues = 0);

	std::string getValueStringFromTable(const char * key, const std::string& defaultValue = "");
	float getValueFloatFromTable(const char * key, float defaultValue = 0.0f);
	int getValueIntegerFromTable(const char * key, int defaultValue = 0);
	bool getValueBoolFromTable(const char * key, bool defaultValue = false);
	void getKeyValueMap(std::map<std::string, std::string>& map, const char *key);

	int getIntValue(const std::string& xpath, int defaultValue = 0);
	float getFloatValue(const std::string& path, float defaultValue = 0.0f);
	std::string getStringFromStack();
	std::string getString(const std::string& expr, const std::string& defaultValue = "");

	static std::string stackDump(lua_State *L);
	std::string stackDump();
};

inline lua_State* LUA::getState() const {
	return _state;
}

inline void LUA::setError(const std::string& error) {
	_error = error;
}

inline const std::string& LUA::getError() const {
	return _error;
}

typedef std::shared_ptr<LUA> LUAPtr;

}
