/**
 * @file
 */

#include "LUA.h"
#include "core/Assert.h"
#include "core/Log.h"
#include <sstream>

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
		core_assert(_startStackDepth == lua_gettop(_state));
	}
};

namespace {
int panicCB(lua_State *L) {
	Log::info("Lua panic. Error message: %s", (lua_isnil(L, -1) ? "" : lua_tostring(L, -1)));
	return 0;
}

void debugHook(lua_State *L, lua_Debug *ar) {
	if (!lua_getinfo(L, "Sn", ar)) {
		return;
	}

	Log::info("LUADBG: %s %s %s %i",
			(ar->namewhat != nullptr) ? ar->namewhat : "",
			(ar->name != nullptr) ? ar->name : "",
			ar->short_src,
			ar->currentline);
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

LUA::LUA(lua_State *state) :
		_state(state), _destroy(false), _debug(false) {
}

LUA::LUA(bool debug) :
		_destroy(true), _debug(debug) {
	openState();
}

LUA::~LUA() {
	closeState();
}

void LUA::openState() {
	_state = luaL_newstate();

	luaL_openlibs(_state);

	// Register panic callback function
	lua_atpanic(_state, panicCB);

	int mask = 0;
	if (_debug) {
		mask |= LUA_MASKCALL;
		mask |= LUA_MASKRET;
		mask |= LUA_MASKLINE;
	}

	// Register debug callback function
	lua_sethook(_state, debugHook, mask, 0);
}

void LUA::closeState() {
	if (_destroy) {
		lua_close(_state);
		_state = nullptr;
	}
}

bool LUA::resetState() {
	// externally managed
	if (!_destroy) {
		return false;
	}

	closeState();
	openState();
	return true;
}

void LUA::reg(const std::string& prefix, const luaL_Reg* funcs) {
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

bool LUA::valueBoolFromTable(const char * key, bool defaultValue) {
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

std::string LUA::valueStringFromTable(const char * key, const std::string& defaultValue) {
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

float LUA::valueFloatFromTable(const char * key, float defaultValue) {
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

int LUA::valueIntegerFromTable(const char * key, int defaultValue) {
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

bool LUA::execute(const std::string &function, int returnValues) {
	lua_getglobal(_state, function.c_str());
	if (lua_isnil(_state, -1)) {
		setError("Function " + function + " wasn't found");
		return false;
	}
	const int ret = lua_pcall(_state, 0, returnValues, 0);
	if (ret != LUA_OK) {
		setError(lua_tostring(_state, -1));
		return false;
	}

	return true;
}

bool LUA::executeUpdate(uint64_t dt) {
	lua_getglobal(_state, "update");
	if (lua_isnil(_state, -1)) {
		setError("Function 'update' wasn't found");
		return false;
	}
	lua_pushinteger(_state, dt);
	const int ret = lua_pcall(_state, 1, 0, 0);
	if (ret != LUA_OK) {
		setError(lua_tostring(_state, -1));
		return false;
	}

	return true;
}

std::string LUA::stackDump(lua_State *L) {
#ifdef DEBUG
	StackChecker check(L);
#endif
	const int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) { /* repeat for each level */
		const int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			lua_pushfstring(L, "%i: %s (%s)", i, lua_tostring(L, i), luaL_typename(L, i));
			break;

		case LUA_TBOOLEAN:
			lua_pushfstring(L, "%i: %s (%s)", i, (lua_toboolean(L, i) ? "true" : "false"), luaL_typename(L, i));
			break;

		case LUA_TNUMBER:
			lua_pushfstring(L, "%i: " LUA_NUMBER_FMT " (%s)", i, lua_tonumber(L, i), luaL_typename(L, i));
			break;

		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
			lua_pushfstring(L, "%i: %p (%s)", i, lua_touserdata(L, i), luaL_typename(L, i));
			break;

		case LUA_TNIL:
			lua_pushfstring(L, "%i: nil", i);
			break;

		default:
			lua_pushfstring(L, "%i: (%s)", i, luaL_typename(L, i));
			break;
		}
	}

	const char* id = lua_tostring(L, -1);
	lua_pop(L, 1);
	if (id == nullptr) {
		return "";
	}
	return id;
}

std::string LUA::stackDump() {
	return stackDump(_state);
}

std::string LUA::stringFromStack() {
	const char* id = lua_tostring(_state, -1);
	pop();
	if (id == nullptr) {
		return "";
	}
	return id;
}

std::string LUA::string(const std::string& expr, const std::string& defaultValue) {
	checkStack();
	const char* r = defaultValue.c_str();
	/* Assign the Lua expression to a Lua global variable. */
	const std::string buf("evalExpr=" + expr);
	if (!luaL_dostring(_state, buf.c_str())) {
		/* Get the value of the global variable */
		lua_getglobal(_state, "evalExpr");
		if (lua_isstring(_state, -1)) {
			r = lua_tostring(_state, -1);
		} else if (lua_isboolean(_state, -1)) {
			r = lua_toboolean(_state, -1) ? "true" : "false";
		}
		/* remove lua_getglobal value */
		lua_pop(_state, 1);
	}
	return r;
}

void LUA::keyValueMap(std::map<std::string, std::string>& map, const char *key) {
	checkStack();
	lua_getglobal(_state, key);
	lua_pushnil(_state);

	while (lua_next(_state, -2) != 0) {
		const char *_key = lua_tostring(_state, -2);
		core_assert(_key);
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

int LUA::intValue(const std::string& path, int defaultValue) {
	const std::string& str = string(path);
	if (str.empty())
		return defaultValue;
	return atoi(str.c_str());
}

float LUA::floatValue(const std::string& path, float defaultValue) {
	const std::string& str = string(path);
	if (str.empty())
		return defaultValue;
	return static_cast<float>(atof(str.c_str()));
}

void LUA::globalKeyValue(const std::string& name) {
	lua_getglobal(_state, name.c_str());
	lua_pushnil(_state);
}

int LUA::table(const std::string& name) {
	lua_getfield(_state, -1, name.c_str());
	return static_cast<int>(lua_rawlen(_state, -1));
}

std::string LUA::tableString(int i) {
	checkStack();
	lua_rawgeti(_state, -1, i);
	const std::string str = lua_tostring(_state, -1);
	pop();
	return str;
}

int LUA::tableInteger(int i) {
	lua_rawgeti(_state, -1, i);
	const int val = static_cast<int>(lua_tointeger(_state, -1));
	pop();
	return val;
}

float LUA::tableFloat(int i) {
	lua_rawgeti(_state, -1, i);
	const float val = static_cast<float>(lua_tonumber(_state, -1));
	pop();
	return val;
}

std::string LUA::key() {
	return lua_tostring(_state, -2);
}

void LUA::pop(int amount) {
	lua_pop(_state, amount);
}

bool LUA::nextKeyValue() {
	return lua_next(_state, -2) != 0;
}

int LUA::global(const std::string& name) {
	return lua_getglobal(_state, name.c_str());
}

}
