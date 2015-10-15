#include "LUA.h"
#include <assert.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>

namespace lua {

class StackChecker {
private:
	lua_State *_state;
	const int _startStackDepth;
public:
	StackChecker(lua_State *state) :
			_state(state), _startStackDepth(lua_gettop(_state)) {
	}
	~StackChecker() {
		assert(_startStackDepth == lua_gettop(_state));
	}
};

namespace {
int panicCB(lua_State *L) {
	std::cout << "Lua panic. Error message: " << (lua_isnil(L, -1) ? "" : lua_tostring(L, -1)) << std::endl << std::flush;
	return 0;
}

void debugHook(lua_State *L, lua_Debug *ar) {
	if (!lua_getinfo(L, "Sn", ar))
		return;

	std::cout << "LUADBG: ";
	if (ar->namewhat != nullptr)
		std::cout << ar->namewhat << " ";
	if (ar->name != nullptr)
		std::cout << ar->name << " ";
	std::cout << ar->short_src << " ";
	std::cout << ar->currentline;
	std::cout << std::endl << std::flush;
}
}

#ifdef DEBUG
#define checkStack() StackChecker(this->_state)
#else
#define checkStack()
#endif

LUAType::LUAType(lua_State* state, const std::string& name) :
		_state(state) {
	const std::string metaTable = META_PREFIX + name;
	luaL_newmetatable(_state, metaTable.c_str());
	lua_pushvalue(_state, -1);
	lua_setfield(_state, -2, "__index");
}

void LUAType::addFunction(const std::string& name, lua_CFunction func) {
	lua_pushcfunction(_state, func);
	lua_setfield(_state, -2, name.c_str());
}

LUA::LUA(bool debug) {
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
}

LUA::~LUA() {
	lua_close(_state);
}

void LUA::reg(const std::string& prefix, luaL_Reg* funcs) {
	const std::string metaTableName = META_PREFIX + prefix;
	luaL_newmetatable(_state, metaTableName.c_str());
	luaL_setfuncs(_state, funcs, 0);
	lua_pushvalue(_state, -1);
	lua_setfield(_state, -1, "__index");
	lua_setglobal(_state, prefix.c_str());
}

LUAType LUA::registerType(const std::string& name) {
	return LUAType(_state, name);
}

bool LUA::load(const std::string& luaString) {
	if (luaL_loadbufferx(_state, luaString.c_str(), luaString.length(), "", nullptr) || lua_pcall(_state, 0, 0, 0)) {
		setError(lua_tostring(_state, -1));
		pop(1);
		return false;
	}

	return true;
}

bool LUA::getValueBoolFromTable(const char * key, bool defaultValue) {
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

std::string LUA::getValueStringFromTable(const char * key, const std::string& defaultValue) {
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const std::string rtn = lua_tostring(_state, -1);
	lua_pop(_state, 1);
	return rtn;
}

float LUA::getValueFloatFromTable(const char * key, float defaultValue) {
	checkStack();
	lua_getfield(_state, -1, key);
	if (lua_isnil(_state, -1)) {
		lua_pop(_state, 1);
		return defaultValue;
	}

	const float rtn = static_cast<float>(lua_tonumber(_state, -1));
	lua_pop(_state, 1);
	return rtn;
}

int LUA::getValueIntegerFromTable(const char * key, int defaultValue) {
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
bool LUA::execute(const std::string &function, int returnValues) {
	lua_getglobal(_state, function.c_str());
	const int ret = lua_pcall(_state, 0, returnValues, 0);
	if (ret != 0) {
		setError(lua_tostring(_state, -1));
		return false;
	}

	return true;
}

std::string LUA::stackDump(lua_State *L) {
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

std::string LUA::stackDump() {
	return stackDump(_state);
}

std::string LUA::getStringFromStack() {
	const char* id = lua_tostring(_state, -1);
	pop();
	if (id == nullptr)
		return "";
	return id;
}

std::string LUA::getString(const std::string& expr, const std::string& defaultValue) {
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

void LUA::getKeyValueMap(std::map<std::string, std::string>& map, const char *key) {
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

int LUA::getIntValue(const std::string& path, int defaultValue) {
	const std::string& str = getString(path);
	if (str.empty())
		return defaultValue;
	return atoi(str.c_str());
}

float LUA::getFloatValue(const std::string& path, float defaultValue) {
	const std::string& str = getString(path);
	if (str.empty())
		return defaultValue;
	return static_cast<float>(atof(str.c_str()));
}

void LUA::getGlobalKeyValue(const std::string& name) {
	lua_getglobal(_state, name.c_str());
	lua_pushnil(_state);
}

int LUA::getTable(const std::string& name) {
	lua_getfield(_state, -1, name.c_str());
	return static_cast<int>(lua_rawlen(_state, -1));
}

std::string LUA::getTableString(int i) {
	checkStack();
	lua_rawgeti(_state, -1, i);
	const std::string str = lua_tostring(_state, -1);
	pop();
	return str;
}

int LUA::getTableInteger(int i) {
	lua_rawgeti(_state, -1, i);
	const int val = static_cast<int>(lua_tointeger(_state, -1));
	pop();
	return val;
}

float LUA::getTableFloat(int i) {
	lua_rawgeti(_state, -1, i);
	const float val = static_cast<float>(lua_tonumber(_state, -1));
	pop();
	return val;
}

std::string LUA::getKey() {
	return lua_tostring(_state, -2);
}

void LUA::pop(int amount) {
	lua_pop(_state, amount);
}

bool LUA::getNextKeyValue() {
	return lua_next(_state, -2) != 0;
}

void LUA::getGlobal(const std::string& name) {
	lua_getglobal(_state, name.c_str());
}

}
