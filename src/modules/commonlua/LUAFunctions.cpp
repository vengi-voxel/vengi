/**
 * @file
 */

#include "LUAFunctions.h"
#include "core/Log.h"
#include "core/command/CommandHandler.h"
#include "core/String.h"
#include "core/Var.h"
#include "core/App.h"
#include "core/io/Filesystem.h"

void clua_assert(lua_State* s, bool pass, const char *msg) {
	if (pass) {
		return;
	}
	lua_Debug ar;
	ar.name = nullptr;
	if (lua_getstack(s, 0, &ar)) {
		lua_getinfo(s, "n", &ar);
	}
	if (ar.name == nullptr) {
		ar.name = "?";
	}
	luaL_error(s, msg, ar.name);
}

void clua_assert_argc(lua_State* s, bool pass) {
	clua_assert(s, pass, "wrong number of arguments to '%s'");
}

int clua_assignmetatable(lua_State* s, const char *name) {
	luaL_getmetatable(s, name);
	if (!lua_istable(s, -1)) {
		Log::error("LUA: metatable for %s doesn't exist", name);
		return 0;
	}
	lua_setmetatable(s, -2);
	return 1;
}

void clua_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name) {
	luaL_newmetatable(s, name);
	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	luaL_setfuncs(s, funcs, 0);
}

void clua_registerfuncsglobal(lua_State* s, const luaL_Reg* funcs, const char *meta, const char *name) {
	luaL_newmetatable(s, meta);
	luaL_setfuncs(s, funcs, 0);
	lua_pushvalue(s, -1);
	lua_setfield(s, -1, "__index");
	lua_setglobal(s, name);
}

bool clua_optboolean(lua_State* s, int index, bool defaultVal) {
	if (lua_isboolean(s, index)) {
		return lua_toboolean(s, index);
	}
	return defaultVal;
}

int clua_typerror(lua_State *s, int narg, const char *tname) {
	const char *msg = lua_pushfstring(s, "%s expected, got %s", tname, luaL_typename(s, narg));
	return luaL_argerror(s, narg, msg);
}

int clua_checkboolean(lua_State *s, int index) {
	if (index < 0) {
		index += lua_gettop(s) + 1;
	}
	luaL_checktype(s, index, LUA_TBOOLEAN);
	return lua_toboolean(s, index);
}

static int clua_cmdexecute(lua_State *s) {
	const char *cmds = luaL_checkstring(s, 1);
	core::executeCommands(cmds);
	return 0;
}

void clua_cmdregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"execute", clua_cmdexecute},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metacmd", "cmd");
}

static int clua_vargetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	lua_pushstring(s, v->strVal().c_str());
	return 1;
}

static int clua_vargetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	lua_pushinteger(s, v->intVal());
	return 1;
}

static int clua_vargetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	lua_pushboolean(s, v->boolVal());
	return 1;
}

static int clua_vargetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	lua_pushnumber(s, v->floatVal());
	return 1;
}

static int clua_varsetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	v->setVal(luaL_checkstring(s, 2));
	return 0;
}

static int clua_varsetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	v->setVal(clua_checkboolean(s, 2));
	return 0;
}

static int clua_varsetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	v->setVal((int)luaL_checkinteger(s, 2));
	return 0;
}

static int clua_varsetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return luaL_error(s, "Invalid variable %s", var);
	}
	v->setVal((float)luaL_checknumber(s, 2));
	return 0;
}

void clua_varregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"str", clua_vargetstr},
		{"bool", clua_vargetbool},
		{"int", clua_vargetint},
		{"float", clua_vargetfloat},
		{"setstr", clua_varsetstr},
		{"setbool", clua_varsetbool},
		{"setint", clua_varsetint},
		{"setfloat", clua_varsetfloat},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metavar", "var");
}

static int clua_loginfo(lua_State *s) {
	Log::info("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logerror(lua_State *s) {
	Log::error("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logwarn(lua_State *s) {
	Log::warn("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logdebug(lua_State *s) {
	Log::debug("%s", luaL_checkstring(s, 1));
	return 0;
}

static int clua_logtrace(lua_State *s) {
	Log::trace("%s", luaL_checkstring(s, 1));
	return 0;
}

void clua_logregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"info", clua_loginfo},
		{"error", clua_logerror},
		{"warn", clua_logwarn},
		{"debug", clua_logdebug},
		{"trace", clua_logtrace},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metalog", "log");
}

int clua_ioloader(lua_State *s) {
	core::String name = luaL_checkstring(s, 1);
	name.replaceAllChars('.', '/');
	name.append(".lua");
	const core::String& content = io::filesystem()->load(name);
	if (luaL_loadbuffer(s, content.c_str(), content.size(), name.c_str())) {
		lua_pop(s, 1);
	}
	return 1;
}

void clua_quatregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"__add", clua_vecadd<glm::quat>},
		{"__sub", clua_vecsub<glm::quat>},
		{"__mul", clua_vecmul<glm::quat>},
		{"__unm", clua_vecnegate<glm::quat>},
		{"__len", clua_veclen<glm::quat>::len},
		{"__index", clua_vecindex<glm::quat>},
		{"__newindex", clua_vecnewindex<glm::quat>},
		{"dot", clua_vecdot<glm::quat>::dot},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<glm::quat>::name());
	clua_registerfuncs(s, funcs, clua_meta<glm::quat>::name());
	const luaL_Reg globalFuncs[] = {
		{"new", clua_vecnew<glm::quat>::vecnew},
		{nullptr, nullptr}
	};
	const core::String& globalMeta = core::string::format("%s_global", clua_meta<glm::quat>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<glm::quat>::name());
}
