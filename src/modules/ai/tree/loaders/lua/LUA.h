#pragma once

#include "common/Types.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <map>
#include <assert.h>
#include <stdlib.h>
#include <sstream>

namespace {
const std::string META_PREFIX = "META_";

int panicCB(lua_State *L) {
	ai_log_error("Lua panic. Error message: %s", (lua_isnil(L, -1) ? "" : lua_tostring(L, -1)));
	return 0;
}

void debugHook(lua_State *L, lua_Debug *ar) {
	if (!lua_getinfo(L, "Sn", ar)) {
		return;
	}

	ai_log("LUADBG: %s %s %s:%i", (ar->namewhat != nullptr) ? ar->namewhat : "", (ar->name != nullptr) ? ar->name : "", ar->short_src, ar->currentline);
}

}

class StackChecker {
private:
	lua_State *_state;
	const int _startStackDepth;
public:
	explicit StackChecker (lua_State *state) :
			_state(state), _startStackDepth(lua_gettop(_state))
	{
	}
	~StackChecker ()
	{
		assert(_startStackDepth == lua_gettop(_state));
	}
};

#ifdef DEBUG
#define checkStack() StackChecker(this->_state)
#else
#define checkStack()
#endif

class LUAType {
private:
	lua_State* _state;
public:
	LUAType(lua_State* state, const std::string& name) :
			_state(state) {
		const std::string metaTable = META_PREFIX + name;
		luaL_newmetatable(_state, metaTable.c_str());
		lua_pushvalue(_state, -1);
		lua_setfield(_state, -2, "__index");
	}

	void addFunction (const std::string& name, lua_CFunction func) {
		lua_pushcfunction(_state, func);
		lua_setfield(_state, -2, name.c_str());
	}

};

class LUA {
private:
	lua_State *_state;
	std::string _error;

public:
	explicit LUA (bool debug = false) {
		_state = luaL_newstate();

		luaL_openlibs(_state);

		// Register panic callback function
		lua_atpanic(_state, panicCB);

		int mask = 0;
		if (debug) {
			mask |= LUA_MASKCALL;
			mask |= LUA_MASKRET;
			mask |= LUA_MASKLINE;
		}

		// Register debug callback function
		lua_sethook(_state, debugHook, mask, 0);

		lua_gc(_state, LUA_GCSTOP, 0);
	}
	~LUA () {
		//const int bytes = lua_gc(_state, LUA_GCCOUNT, 0) * 1024 + lua_gc(_state, LUA_GCCOUNTB, 0);
		lua_gc(_state, LUA_GCRESTART, 0);
		lua_close(_state);
	}

	inline lua_State* getState () const;

	template<class T>
	static void newGlobalData (lua_State *L, const std::string& prefix, T *userData)
	{
		lua_pushlightuserdata(L, userData);
		lua_setglobal(L, prefix.c_str());
	}

	template<class T>
	inline void newGlobalData (const std::string& prefix, T *userData) const
	{
		newGlobalData(_state, prefix, userData);
	}

	template<class T>
	static T* getGlobalData(lua_State *L, const std::string& prefix)
	{
		lua_getglobal(L, prefix.c_str());
		T* data = (T*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return data;
	}

	template<class T>
	inline T* getGlobalData(const std::string& prefix) const
	{
		return getGlobalData<T>(_state, prefix);
	}

	template<class T>
	static T** newUserdata (lua_State *L, const std::string& prefix)
	{
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const std::string name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		return udata;
	}

	template<class T>
	static T* getUserData (lua_State *L, int n, const std::string& prefix)
	{
		const std::string name = META_PREFIX + prefix;
		return *(T **) luaL_checkudata(L, n, name.c_str());
	}

	static void returnError (lua_State *L, const std::string& error)
	{
		ai_log_error("LUA error: '%s'", error.c_str());
		luaL_error(L, "%s", error.c_str());
	}

	void getGlobal (const std::string& name);

	std::string getKey ();

	void getGlobalKeyValue (const std::string& name);

	bool getNextKeyValue ();

	void pop (int amount = 1);

	int getTable (const std::string& name);

	std::string getTableString (int i);

	int getTableInteger (int i);

	float getTableFloat (int i);

	void reg (const std::string& prefix, luaL_Reg* funcs);
	LUAType registerType (const std::string& name);

	void setError (const std::string& error);
	const std::string& getError () const;
	bool load (const std::string &file);
	/**
	 * @param[in] function function to be called
	 */
	bool execute (const std::string &function, int returnValues = 0);

	std::string getValueStringFromTable (const char * key, const std::string& defaultValue = "");
	float getValueFloatFromTable (const char * key, float defaultValue = 0.0f);
	int getValueIntegerFromTable (const char * key, int defaultValue = 0);
	bool getValueBoolFromTable (const char * key, bool defaultValue = false);
	void getKeyValueMap (std::map<std::string, std::string>& map, const char *key);

	int getIntValue (const std::string& xpath, int defaultValue = 0);
	float getFloatValue (const std::string& path, float defaultValue = 0.0f);
	std::string getStringFromStack ();
	std::string getString (const std::string& expr, const std::string& defaultValue = "");

	static std::string stackDump (lua_State *L);
	std::string stackDump ();
};

inline lua_State* LUA::getState () const
{
	return _state;
}

inline void LUA::setError (const std::string& error)
{
	_error = error;
}

inline const std::string& LUA::getError () const
{
	return _error;
}

inline void LUA::reg (const std::string& prefix, luaL_Reg* funcs)
{
	const std::string metaTableName = META_PREFIX + prefix;
	luaL_newmetatable(_state, metaTableName.c_str());
	luaL_setfuncs(_state, funcs, 0);
	lua_pushvalue(_state, -1);
	lua_setfield(_state, -1, "__index");
	lua_setglobal(_state, prefix.c_str());
}

inline LUAType LUA::registerType (const std::string& name)
{
	return LUAType(_state, name);
}

inline bool LUA::load (const std::string& luaString)
{
	if (luaL_loadbufferx(_state, luaString.c_str(), luaString.length(), "", nullptr) || lua_pcall(_state, 0, 0, 0)) {
		setError(lua_tostring(_state, -1));
		pop(1);
		return false;
	}

	return true;
}

inline bool LUA::getValueBoolFromTable (const char * key, bool defaultValue)
{
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const bool rtn = lua_toboolean(_state, -1);
	lua_pop(_state, 1);
	return rtn;
}

inline std::string LUA::getValueStringFromTable (const char * key, const std::string& defaultValue)
{
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const std::string rtn = lua_tostring(_state,-1);
	lua_pop(_state, 1);
	return rtn;
}

inline float LUA::getValueFloatFromTable (const char * key, float defaultValue)
{
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const float rtn = static_cast<float>(lua_tonumber(_state,-1));
	lua_pop(_state, 1);
	return rtn;
}

inline int LUA::getValueIntegerFromTable (const char * key, int defaultValue)
{
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const int rtn = static_cast<int>(lua_tointeger(_state, -1));
	lua_pop(_state, 1);
	return rtn;
}

/**
 * @param[in] function function to be called
 */
inline bool LUA::execute (const std::string &function, int returnValues)
{
	lua_getglobal(_state, function.c_str());
	const int ret = lua_pcall(_state, 0, returnValues, 0);
	if (ret != 0) {
		setError(lua_tostring(_state, -1));
		return false;
	}

	return true;
}

inline std::string LUA::stackDump (lua_State *L)
{
#ifdef DEBUG
	StackChecker check(L);
#endif
	std::stringstream ss;
	const int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) { /* repeat for each level */
		const int t = lua_type(L, i);
		ss << i << ": ";
		switch (t) {
		case LUA_TSTRING:
			ss << lua_typename(L, t) << ": '" << lua_tostring(L, i) << "'";
			break;

		case LUA_TBOOLEAN:
			ss << lua_typename(L, t) << ": '" << (lua_toboolean(L, i) ? "true" : "false") << "'";
			break;

		case LUA_TNUMBER:
			ss << lua_typename(L, t) << ": '" << lua_tonumber(L, i) << "'";
			break;

		case LUA_TUSERDATA:
			ss << lua_typename(L, t) << ": '" << lua_touserdata(L, i) << "'";
			break;

		case LUA_TLIGHTUSERDATA:
			ss << lua_typename(L, t) << ": '" << lua_touserdata(L, i) << "'";
			break;

		case LUA_TTABLE:
			ss << lua_typename(L, t) << ": '...'";
			break;

		case LUA_TFUNCTION:
			ss << lua_typename(L, t) << ": '...'";
			break;

		case LUA_TTHREAD:
			ss << lua_typename(L, t) << ": '...'";
			break;

		default:
			ss << lua_typename(L, t);
			break;

		}
		ss << std::endl;
	}

	return ss.str();
}

inline std::string LUA::stackDump ()
{
	return stackDump(_state);
}

inline std::string LUA::getStringFromStack ()
{
	const char* id = lua_tostring(_state, -1);
	pop();
	if (id == nullptr)
		return "";
	return id;
}

inline std::string LUA::getString (const std::string& expr, const std::string& defaultValue)
{
	checkStack();
	const char* r = defaultValue.c_str();
	/* Assign the Lua expression to a Lua global variable. */
	const std::string buf("evalExpr=" + expr);
	if (!luaL_dostring(_state, buf.c_str())) {
		/* Get the value of the global variable */
		lua_getglobal(_state, "evalExpr");
		if (lua_isstring(_state, -1))
			r = lua_tostring(_state, -1);
		else if (lua_isboolean(_state, -1))
			r = lua_toboolean(_state, -1) ? "true" : "false";
		/* remove lua_getglobal value */
		lua_pop(_state, 1);
	}
	return r;
}

inline void LUA::getKeyValueMap (std::map<std::string, std::string>& map, const char *key)
{
	checkStack();
	lua_getglobal(_state, key);
	lua_pushnil(_state);

	while (lua_next(_state, -2) != 0) {
		const char *_key = lua_tostring(_state, -2);
		assert(_key);
		std::string _value;
		if (lua_isstring(_state, -1)) {
			_value = lua_tostring(_state, -1);
		} else if (lua_isnumber(_state, -1)) {
			std::stringstream ss;
			ss << lua_tonumber(_state, -1);
			_value = ss.str();
		} else if (lua_isboolean(_state, -1)) {
			_value = lua_toboolean(_state, -1) ? "true" : "false";
		}
		map[_key] = _value;
		lua_pop(_state, 1);
	}

	lua_pop(_state, 1);
}

inline int LUA::getIntValue (const std::string& path, int defaultValue)
{
	const std::string& str = getString(path);
	if (str.empty())
		return defaultValue;
	return atoi(str.c_str());
}

inline float LUA::getFloatValue (const std::string& path, float defaultValue)
{
	const std::string& str = getString(path);
	if (str.empty())
		return defaultValue;
	return static_cast<float>(atof(str.c_str()));
}

inline void LUA::getGlobalKeyValue (const std::string& name)
{
	lua_getglobal(_state, name.c_str());
	lua_pushnil(_state);
}

inline int LUA::getTable (const std::string& name)
{
	lua_getfield(_state, -1, name.c_str());
	return static_cast<int>(lua_rawlen(_state, -1));
}

inline std::string LUA::getTableString (int i)
{
	checkStack();
	lua_rawgeti(_state, -1, i);
	const std::string str = lua_tostring(_state, -1);
	pop();
	return str;
}

inline int LUA::getTableInteger (int i)
{
	lua_rawgeti(_state, -1, i);
	const int val = static_cast<int>(lua_tointeger(_state, -1));
	pop();
	return val;
}

inline float LUA::getTableFloat (int i)
{
	lua_rawgeti(_state, -1, i);
	const float val = static_cast<float>(lua_tonumber(_state, -1));
	pop();
	return val;
}

inline std::string LUA::getKey ()
{
	return lua_tostring(_state, -2);
}

inline void LUA::pop (int amount)
{
	lua_pop(_state, amount);
}

inline bool LUA::getNextKeyValue ()
{
	return lua_next(_state, -2) != 0;
}

inline void LUA::getGlobal (const std::string& name)
{
	lua_getglobal(_state, name.c_str());
}
