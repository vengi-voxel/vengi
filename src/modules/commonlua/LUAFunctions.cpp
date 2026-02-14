/**
 * @file
 */

#include "LUAFunctions.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/GLMConst.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/Var.h"
#include "core/collection/StringSet.h"
#include "http/Http.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>

int clua_errorhandler(lua_State* s) {
	Log::error("Lua error handler invoked");
	return 0;
}

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
	clua_error(s, msg, ar.name);
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

bool clua_registernew(lua_State* s, const char *name, lua_CFunction func) {
	if (luaL_getmetatable(s, name) == 0) {
		Log::error("Could not find metatable for %s", name);
		return false;
	}
	// Set a metatable for the metatable
	// This allows using Object(42) to make new objects
	lua_newtable(s);
	lua_pushcfunction(s, func);
	lua_setfield(s, -2, "__call");
	lua_setmetatable(s, -2);
	return true;
}

#ifdef DEBUG
static bool clua_validatefuncs(const luaL_Reg* funcs) {
	core::StringSet funcSet;
	for (; funcs->name != nullptr; ++funcs) {
		if (!funcSet.insert(funcs->name)) {
			Log::error("%s is already in the given funcs", funcs->name);
			return false;
		}
	}
	return true;
}
#endif

bool clua_registerfuncs(lua_State *s, const luaL_Reg *funcs, const char *name) {
	if (luaL_newmetatable(s, name) == 0) {
		Log::warn("Metatable %s already exists", name);
		return false;
	}
#ifdef DEBUG
	if (!clua_validatefuncs(funcs)) {
		return false;
	}
#endif
	luaL_setfuncs(s, funcs, 0);
	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	lua_pop(s, 1);
	return true;
}

// Helper function to create the jsonhelp metatable name
static core::String clua_jsonhelpname(const char *name) {
	return core::String::format("%s_jsonhelp", name);
}

bool clua_registerfuncs(lua_State *s, const clua_Reg *funcs, const char *name) {
	if (luaL_newmetatable(s, name) == 0) {
		Log::warn("Metatable %s already exists", name);
		return false;
	}

	// Register the functions
	for (const clua_Reg *f = funcs; f->name != nullptr; ++f) {
		lua_pushcfunction(s, f->func);
		lua_setfield(s, -2, f->name);
	}

	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	lua_pop(s, 1);

	// Create a separate metatable to store jsonhelp functions
	const core::String jsonHelpMetaName = clua_jsonhelpname(name);
	luaL_newmetatable(s, jsonHelpMetaName.c_str());
	for (const clua_Reg *f = funcs; f->name != nullptr; ++f) {
		if (f->jsonHelp != nullptr) {
			lua_pushcfunction(s, f->jsonHelp);
			lua_setfield(s, -2, f->name);
		}
	}
	lua_pop(s, 1);

	return true;
}

bool clua_registerfuncsglobal(lua_State* s, const luaL_Reg* funcs, const char *meta, const char *name) {
	if (luaL_newmetatable(s, meta) == 0) {
		Log::warn("Metatable %s already exists", meta);
		return false;
	}
#ifdef DEBUG
	if (!clua_validatefuncs(funcs)) {
		return false;
	}
#endif
	luaL_setfuncs(s, funcs, 0);
	lua_pushvalue(s, -1);
	lua_setfield(s, -1, "__index");
	lua_setglobal(s, name);
	return true;
}

bool clua_registerfuncsglobal(lua_State* s, const clua_Reg* funcs, const char *meta, const char *name) {
	if (luaL_newmetatable(s, meta) == 0) {
		Log::warn("Metatable %s already exists", meta);
		return false;
	}

	// Register the functions
	for (const clua_Reg *f = funcs; f->name != nullptr; ++f) {
		lua_pushcfunction(s, f->func);
		lua_setfield(s, -2, f->name);
	}

	lua_pushvalue(s, -1);
	lua_setfield(s, -1, "__index");
	lua_setglobal(s, name);

	// Create a separate metatable to store jsonhelp functions
	const core::String jsonHelpMetaName = clua_jsonhelpname(meta);
	luaL_newmetatable(s, jsonHelpMetaName.c_str());
	for (const clua_Reg *f = funcs; f->name != nullptr; ++f) {
		if (f->jsonHelp != nullptr) {
			lua_pushcfunction(s, f->jsonHelp);
			lua_setfield(s, -2, f->name);
		}
	}
	lua_pop(s, 1);

	return true;
}

lua_CFunction clua_getjsonhelp(lua_State* s, const char *name, const char *method) {
	const core::String jsonHelpMetaName = clua_jsonhelpname(name);
	if (luaL_getmetatable(s, jsonHelpMetaName.c_str()) == 0) {
		lua_pop(s, 1);
		return nullptr;
	}
	lua_getfield(s, -1, method);
	lua_CFunction func = nullptr;
	if (lua_iscfunction(s, -1)) {
		func = lua_tocfunction(s, -1);
	}
	lua_pop(s, 2);
	return func;
}

static core::String clua_stackdump(lua_State *L) {
	constexpr int depth = 64;
	core::String dump;
	dump.reserve(1024);
	dump.append("Stacktrace:\n");
	for (int cnt = 0; cnt < depth; cnt++) {
		lua_Debug dbg;
		if (lua_getstack(L, cnt + 1, &dbg) == 0) {
			break;
		}
		lua_getinfo(L, "Snl", &dbg);
		const char *func = dbg.name ? dbg.name : dbg.short_src;
		dump.append(cnt);
		dump.append(": ");
		dump.append(func);
		dump.append("\n");
	}
	dump.append("\n");
	const int top = lua_gettop(L);
	dump.append(core::String::format("%i values on stack%c", top, top > 0 ? '\n' : ' '));

	for (int i = 1; i <= top; i++) { /* repeat for each level */
		const int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			lua_pushfstring(L, "%d: %s (%s)\n", i, lua_tostring(L, i), luaL_typename(L, i));
			break;

		case LUA_TBOOLEAN:
			lua_pushfstring(L, "%d: %s (%s)\n", i, (lua_toboolean(L, i) ? "true" : "false"), luaL_typename(L, i));
			break;

		case LUA_TNUMBER:
			lua_pushfstring(L, "%d: %f (%s)\n", i, lua_tonumber(L, i), luaL_typename(L, i));
			break;

		case LUA_TUSERDATA:
		case LUA_TLIGHTUSERDATA:
			lua_pushfstring(L, "%d: %p (%s)\n", i, lua_touserdata(L, i), luaL_typename(L, i));
			break;

		case LUA_TNIL:
			lua_pushfstring(L, "%d: nil\n", i);
			break;

		default:
			lua_pushfstring(L, "%d: (%s)\n", i, luaL_typename(L, i));
			break;
		}
		const char* id = lua_tostring(L, -1);
		if (id != nullptr) {
			dump.append(id);
		}
		lua_pop(L, 1);
	}

	return dump;
}

void clua_error_prepare(lua_State *s, const char *fmt, ...) {
	core::String stackdump = clua_stackdump(s);
	Log::error("%s", stackdump.c_str());
	stackdump = core::String();
	va_list argp;
	va_start(argp, fmt);
	luaL_where(s, 1);
	lua_pushvfstring(s, fmt, argp);
	va_end(argp);
	lua_concat(s, 2);
}

int clua_error(lua_State *s, const char *fmt, ...) {
	core::String stackdump = clua_stackdump(s);
	Log::error("%s", stackdump.c_str());
	stackdump = core::String();
	va_list argp;
	va_start(argp, fmt);
	luaL_where(s, 1);
	lua_pushvfstring(s, fmt, argp);
	va_end(argp);
	lua_concat(s, 2);
	return lua_error(s);
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
	command::executeCommands(cmds);
	return 0;
}

static int clua_cmd_execute_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "execute",
		"summary": "Execute a command string.",
		"parameters": [
			{"name": "cmdline", "type": "string", "description": "The command line to execute."}
		],
		"returns": []})");
	return 1;
}

void clua_cmdregister(lua_State* s) {
	static const clua_Reg funcs[] = {
		{"execute", clua_cmdexecute, clua_cmd_execute_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, clua_metacmd(), "g_cmd");
}

static int clua_varcreate(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const char *val = luaL_checkstring(s, 2);
	const char *help = luaL_optstring(s, 3, nullptr);
	bool noperist = clua_optboolean(s, 4, false);
	bool secret = clua_optboolean(s, 5, false);
	uint32_t flags = 0;
	if (noperist) {
		flags |= core::CV_NOPERSIST;
	}
	if (secret) {
		flags |= core::CV_SECRET;
	}
	core::Var::get(var, val, flags, help);
	return 0;
}

static int clua_vargetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushstring(s, v->strVal().c_str());
	return 1;
}

static int clua_vargetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushinteger(s, v->intVal());
	return 1;
}

static int clua_vargetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushboolean(s, v->boolVal());
	return 1;
}

static int clua_vargetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	lua_pushnumber(s, v->floatVal());
	return 1;
}

static int clua_varsetstr(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal(luaL_checkstring(s, 2));
	return 0;
}

static int clua_varsetbool(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal(clua_checkboolean(s, 2));
	return 0;
}

static int clua_varsetint(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal((int)luaL_checkinteger(s, 2));
	return 0;
}

static int clua_varsetfloat(lua_State *s) {
	const char *var = luaL_checkstring(s, 1);
	const core::VarPtr& v = core::Var::get(var, nullptr);
	if (!v) {
		return clua_error(s, "Invalid variable %s", var);
	}
	v->setVal((float)luaL_checknumber(s, 2));
	return 0;
}

static int clua_var_create_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "create",
		"summary": "Create a new cvar that is persisted by default.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."},
			{"name": "value", "type": "string", "description": "The initial value."},
			{"name": "help", "type": "string", "description": "Help text (optional)."},
			{"name": "nopersist", "type": "boolean", "description": "If true, the cvar won't be persisted (optional)."},
			{"name": "secret", "type": "boolean", "description": "If true, the cvar value is hidden (optional)."}
		],
		"returns": []})");
	return 1;
}

static int clua_var_str_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "str",
		"summary": "Get the string value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."}
		],
		"returns": [
			{"type": "string", "description": "The cvar's string value."}
		]})");
	return 1;
}

static int clua_var_bool_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "bool",
		"summary": "Get the boolean value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."}
		],
		"returns": [
			{"type": "boolean", "description": "The cvar's boolean value."}
		]})");
	return 1;
}

static int clua_var_int_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "int",
		"summary": "Get the integer value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."}
		],
		"returns": [
			{"type": "integer", "description": "The cvar's integer value."}
		]})");
	return 1;
}

static int clua_var_float_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "float",
		"summary": "Get the float value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."}
		],
		"returns": [
			{"type": "number", "description": "The cvar's float value."}
		]})");
	return 1;
}

static int clua_var_setstr_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "setStr",
		"summary": "Set the string value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."},
			{"name": "value", "type": "string", "description": "The new string value."}
		],
		"returns": []})");
	return 1;
}

static int clua_var_setbool_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "setBool",
		"summary": "Set the boolean value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."},
			{"name": "value", "type": "boolean", "description": "The new boolean value."}
		],
		"returns": []})");
	return 1;
}

static int clua_var_setint_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "setInt",
		"summary": "Set the integer value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."},
			{"name": "value", "type": "integer", "description": "The new integer value."}
		],
		"returns": []})");
	return 1;
}

static int clua_var_setfloat_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "setFloat",
		"summary": "Set the float value of a cvar.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The cvar name."},
			{"name": "value", "type": "number", "description": "The new float value."}
		],
		"returns": []})");
	return 1;
}

void clua_varregister(lua_State* s) {
	static const clua_Reg funcs[] = {
		{"create", clua_varcreate, clua_var_create_jsonhelp},
		{"str", clua_vargetstr, clua_var_str_jsonhelp},
		{"bool", clua_vargetbool, clua_var_bool_jsonhelp},
		{"int", clua_vargetint, clua_var_int_jsonhelp},
		{"float", clua_vargetfloat, clua_var_float_jsonhelp},
		{"setStr", clua_varsetstr, clua_var_setstr_jsonhelp},
		{"setBool", clua_varsetbool, clua_var_setbool_jsonhelp},
		{"setInt", clua_varsetint, clua_var_setint_jsonhelp},
		{"setFloat", clua_varsetfloat, clua_var_setfloat_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, clua_metavar(), "g_var");
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

static int clua_log_info_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "info",
		"summary": "Log an info message.",
		"parameters": [
			{"name": "message", "type": "string", "description": "The message to log."}
		],
		"returns": []})");
	return 1;
}

static int clua_log_error_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "error",
		"summary": "Log an error message.",
		"parameters": [
			{"name": "message", "type": "string", "description": "The message to log."}
		],
		"returns": []})");
	return 1;
}

static int clua_log_warn_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "warn",
		"summary": "Log a warning message.",
		"parameters": [
			{"name": "message", "type": "string", "description": "The message to log."}
		],
		"returns": []})");
	return 1;
}

static int clua_log_debug_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "debug",
		"summary": "Log a debug message.",
		"parameters": [
			{"name": "message", "type": "string", "description": "The message to log."}
		],
		"returns": []})");
	return 1;
}

static int clua_log_trace_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "trace",
		"summary": "Log a trace message.",
		"parameters": [
			{"name": "message", "type": "string", "description": "The message to log."}
		],
		"returns": []})");
	return 1;
}

void clua_logregister(lua_State* s) {
	static const clua_Reg funcs[] = {
		{"info", clua_loginfo, clua_log_info_jsonhelp},
		{"error", clua_logerror, clua_log_error_jsonhelp},
		{"warn", clua_logwarn, clua_log_warn_jsonhelp},
		{"debug", clua_logdebug, clua_log_debug_jsonhelp},
		{"trace", clua_logtrace, clua_log_trace_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, clua_metalog(), "g_log");
}

int clua_ioloader(lua_State *s) {
	core::String name = luaL_checkstring(s, 1);
	name.replaceAllChars('.', '/');
	name.append(".lua");
	io::FilePtr file = io::filesystem()->open(name);
	if (!file->exists()) {
		file.release();
		return clua_error(s, "Could not open required file %s", name.c_str());
	}
	const core::String& content = file->load();
	Log::debug("Loading lua module %s with %i bytes", name.c_str(), (int)content.size());
	if (luaL_loadbuffer(s, content.c_str(), content.size(), name.c_str())) {
		Log::error("%s", lua_tostring(s, -1));
		lua_pop(s, 1);
	}
	return 1;
}

bool clua_isquat(lua_State *s, int n) {
	return lua_istable(s, n);
}

glm::quat clua_toquat(lua_State *s, int n) {
	luaL_checktype(s, n, LUA_TTABLE);
	glm::quat v;
	lua_getfield(s, n, VEC_MEMBERS[0]);
	v.x = LuaNumberFuncs<typename glm::quat::value_type>::check(s, -1);
	lua_pop(s, 1);
	lua_getfield(s, n, VEC_MEMBERS[1]);
	v.y = LuaNumberFuncs<typename glm::quat::value_type>::check(s, -1);
	lua_pop(s, 1);
	lua_getfield(s, n, VEC_MEMBERS[2]);
	v.z = LuaNumberFuncs<typename glm::quat::value_type>::check(s, -1);
	lua_pop(s, 1);
	lua_getfield(s, n, VEC_MEMBERS[3]);
	v.w = LuaNumberFuncs<typename glm::quat::value_type>::check(s, -1);
	lua_pop(s, 1);
	return v;
}

static glm::quat rotateX(float angle) {
	return glm::angleAxis(angle, glm::right());
}

static glm::quat rotateZ(float angle) {
	return glm::angleAxis(angle, glm::backward());
}

static glm::quat rotateY(float angle) {
	return glm::angleAxis(angle, glm::up());
}

static int clua_quat_rotate_xyz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float y = lua_tonumber(s, 2);
	const float z = lua_tonumber(s, 3);
	return clua_push(s, glm::quat(glm::vec3(x, y, z)));
}

static int clua_quat_rotate_xy(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float y = lua_tonumber(s, 2);
	return clua_push(s, glm::quat(glm::vec3(x, y, 0.0f)));
}

static int clua_quat_rotate_yz(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, glm::quat(glm::vec3(0.0f, y, z)));
}

static int clua_quat_rotate_xz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, glm::quat(glm::vec3(x, 0.0f, z)));
}

static int clua_quat_rotate_x(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	return clua_push(s, rotateX(x));
}

static int clua_quat_rotate_y(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	return clua_push(s, rotateY(y));
}

static int clua_quat_rotate_z(lua_State* s) {
	const float z = lua_tonumber(s, 1);
	return clua_push(s, rotateZ(z));
}

static int clua_quat_new(lua_State* s) {
	glm::quat array = glm::quat_identity<float, glm::defaultp>();
	return clua_push(s, array);
}

static int clua_quatmul(lua_State* s) {
	const glm::quat& a = clua_toquat(s, 1);
	const glm::quat& b = clua_toquat(s, 2);
	const glm::quat& c = a * b;
	return clua_push(s, c);
}

static int clua_quat_slerp(lua_State* s) {
	const glm::quat& a = clua_toquat(s, 1);
	const glm::quat& b = clua_toquat(s, 2);
	const float t = lua_tonumber(s, 3);
	return clua_push(s, glm::slerp(a, b, t));
}

static int clua_quat_conjugate(lua_State* s) {
	const glm::quat& q = clua_toquat(s, 1);
	return clua_push(s, glm::conjugate(q));
}

static int clua_quat_fromaxisangle(lua_State* s) {
	const glm::vec3 axis = clua_tovec<glm::vec3>(s, 1);
	const float angle = lua_tonumber(s, 2);
	return clua_push(s, glm::angleAxis(angle, glm::normalize(axis)));
}

static int clua_quatindex(lua_State *s) {
	const glm::quat& v = clua_toquat(s, 1);
	const char* i = luaL_checkstring(s, 2);

	switch (*i) {
	case '0':
	case 'x':
		lua_pushnumber(s, v.x);
		return 1;
	case '1':
	case 'y':
		lua_pushnumber(s, v.y);
		return 1;
	case '2':
	case 'z':
		lua_pushnumber(s, v.z);
		return 1;
	case '3':
	case 'w':
		lua_pushnumber(s, v.w);
		return 1;
	default:
		break;
	}
	return clua_error(s, "Invalid component %c", *i);
}

static int clua_quat_new_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "new",
		"summary": "Create a new identity quaternion.",
		"parameters": [],
		"returns": [
			{"type": "quat", "description": "A new identity quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatexyz_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateXYZ",
		"summary": "Create a quaternion rotation around X, Y, and Z axes (pitch, yaw, roll).",
		"parameters": [
			{"name": "x", "type": "number", "description": "Rotation angle around X axis in radians."},
			{"name": "y", "type": "number", "description": "Rotation angle around Y axis in radians."},
			{"name": "z", "type": "number", "description": "Rotation angle around Z axis in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatexy_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateXY",
		"summary": "Create a quaternion rotation around X and Y axes (pitch and yaw).",
		"parameters": [
			{"name": "x", "type": "number", "description": "Rotation angle around X axis in radians."},
			{"name": "y", "type": "number", "description": "Rotation angle around Y axis in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotateyz_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateYZ",
		"summary": "Create a quaternion rotation around Y and Z axes (yaw and roll).",
		"parameters": [
			{"name": "y", "type": "number", "description": "Rotation angle around Y axis in radians."},
			{"name": "z", "type": "number", "description": "Rotation angle around Z axis in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatexz_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateXZ",
		"summary": "Create a quaternion rotation around X and Z axes (roll and pitch).",
		"parameters": [
			{"name": "x", "type": "number", "description": "Rotation angle around X axis in radians."},
			{"name": "z", "type": "number", "description": "Rotation angle around Z axis in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatex_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateX",
		"summary": "Create a quaternion rotation around the X axis (pitch - nod forward/backward).",
		"parameters": [
			{"name": "angle", "type": "number", "description": "Rotation angle in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatey_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateY",
		"summary": "Create a quaternion rotation around the Y axis (yaw - turn left/right).",
		"parameters": [
			{"name": "angle", "type": "number", "description": "Rotation angle in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_rotatez_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "rotateZ",
		"summary": "Create a quaternion rotation around the Z axis (roll - tilt head left/right).",
		"parameters": [
			{"name": "angle", "type": "number", "description": "Rotation angle in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

static int clua_quat_slerp_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "slerp",
		"summary": "Spherical linear interpolation between two quaternions.",
		"parameters": [
			{"name": "a", "type": "quat", "description": "The start quaternion."},
			{"name": "b", "type": "quat", "description": "The end quaternion."},
			{"name": "t", "type": "number", "description": "Interpolation factor (0.0 to 1.0)."}
		],
		"returns": [
			{"type": "quat", "description": "The interpolated quaternion."}
		]})");
	return 1;
}

static int clua_quat_conjugate_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "conjugate",
		"summary": "Get the conjugate (inverse rotation) of a quaternion.",
		"parameters": [
			{"name": "q", "type": "quat", "description": "The quaternion."}
		],
		"returns": [
			{"type": "quat", "description": "The conjugated quaternion."}
		]})");
	return 1;
}

static int clua_quat_fromaxisangle_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "fromAxisAngle",
		"summary": "Create a quaternion from an axis and an angle.",
		"parameters": [
			{"name": "axis", "type": "vec3", "description": "The rotation axis (will be normalized)."},
			{"name": "angle", "type": "number", "description": "Rotation angle in radians."}
		],
		"returns": [
			{"type": "quat", "description": "The rotation quaternion."}
		]})");
	return 1;
}

void clua_quatregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"__mul", clua_quatmul},
		{"__index", clua_quatindex},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<glm::quat>::name());
	clua_registerfuncs(s, funcs, clua_meta<glm::quat>::name());
	static const clua_Reg globalFuncs[] = {
		{"new",          clua_quat_new, clua_quat_new_jsonhelp},
		{"rotateXYZ",    clua_quat_rotate_xyz, clua_quat_rotatexyz_jsonhelp},
		{"rotateXY",     clua_quat_rotate_xy, clua_quat_rotatexy_jsonhelp},
		{"rotateYZ",     clua_quat_rotate_yz, clua_quat_rotateyz_jsonhelp},
		{"rotateXZ",     clua_quat_rotate_xz, clua_quat_rotatexz_jsonhelp},
		{"rotateX",      clua_quat_rotate_x, clua_quat_rotatex_jsonhelp},
		{"rotateY",      clua_quat_rotate_y, clua_quat_rotatey_jsonhelp},
		{"rotateZ",      clua_quat_rotate_z, clua_quat_rotatez_jsonhelp},
		{"slerp",        clua_quat_slerp, clua_quat_slerp_jsonhelp},
		{"conjugate",    clua_quat_conjugate, clua_quat_conjugate_jsonhelp},
		{"fromAxisAngle", clua_quat_fromaxisangle, clua_quat_fromaxisangle_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	const core::String& globalMeta = core::String::format("%s_global", clua_meta<glm::quat>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<glm::quat>::name());
}

static int clua_sleep_continuation(lua_State *s, int status, lua_KContext ctx) {
	app::App *app = app::App::getInstance();
	const core::TimeProviderPtr &timeProvider = app->timeProvider();
	const uint64_t currentMillis = timeProvider->systemMillis();
	const uint64_t wakeupTime = (uint64_t)lua_tointeger(s, 2);

	if (currentMillis >= wakeupTime) {
		return 0;
	}

	return lua_yieldk(s, 0, 0, clua_sleep_continuation);
}

static int clua_syssleep(lua_State *s) {
	const int ms = luaL_checkinteger(s, 1);
	app::App* app = app::App::getInstance();
	if (!lua_isyieldable(s)) {
		app->wait(ms);
		return 0;
	}

	const core::TimeProviderPtr &timeProvider = app->timeProvider();
	const uint64_t currentMillis = timeProvider->systemMillis();

	if (ms > 0) {
		lua_pushinteger(s, (lua_Integer)(currentMillis + (uint64_t)ms));
		return lua_yieldk(s, 0, 0, clua_sleep_continuation);
	}
	return 0;
}

static int clua_sysshouldquit(lua_State *s) {
	app::App* app = app::App::getInstance();
	lua_pushboolean(s, app->shouldQuit());
	return 1;
}

static int clua_sys_sleep_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "sleep",
		"summary": "Sleep for the specified number of milliseconds.",
		"parameters": [
			{"name": "ms", "type": "integer", "description": "The number of milliseconds to sleep."}
		],
		"returns": []})");
	return 1;
}

static int clua_sys_shouldquit_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "shouldQuit",
		"summary": "Check if the application should quit.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if the application should quit."}
		]})");
	return 1;
}

static void clua_sysregister(lua_State *s) {
	static const clua_Reg funcs[] = {
		{"sleep", clua_syssleep, clua_sys_sleep_jsonhelp},
		{"shouldQuit", clua_sysshouldquit, clua_sys_shouldquit_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, clua_metasys(), "g_sys");
}

static int clua_io_sysopen(lua_State *s) {
	const char *path = luaL_checkstring(s, 1);
	const char *modeStr = luaL_optstring(s, 2, "r");
	io::FileMode mode;
	if (modeStr[0] == 'r') {
		mode = io::FileMode::SysRead;
	} else if (modeStr[0] == 'w') {
		mode = io::FileMode::SysWrite;
	} else {
		return clua_error(s, "Invalid mode %s", modeStr);
	}
	if (mode == io::FileMode::SysWrite) {
		if (!io::Filesystem::sysIsWriteable(path)) {
			return clua_error(s, "Could not open file %s for writing with mode %s", path, modeStr);
		}
	} else if (!io::filesystem()->exists(path)) {
		return clua_error(s, "Could not open file %s in mode %s", path, modeStr);
	}
	clua_pushstream(s, new io::FileStream(io::filesystem()->open(path, mode)));
	return 1;
}

static int clua_io_open(lua_State *s) {
	const char *path = luaL_checkstring(s, 1);
	const char *modeStr = luaL_optstring(s, 2, "r");
	io::FileMode mode;
	if (modeStr[0] == 'r') {
		mode = io::FileMode::Read;
	} else if (modeStr[0] == 'w') {
		mode = io::FileMode::Write;
	} else {
		return clua_error(s, "Invalid mode %s", modeStr);
	}
	if (mode == io::FileMode::Read) {
		if (!io::filesystem()->exists(path)) {
			return clua_error(s, "Could not open file %s in mode %s", path, modeStr);
		}
	}
	clua_pushstream(s, new io::FileStream(io::filesystem()->open(path, mode)));
	return 1;
}

const char *clua_metaio() {
	return "__global_io";
}

static int clua_io_sysopen_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "sysopen",
		"summary": "Open a file from an absolute path or relative to the current working directory.",
		"parameters": [
			{"name": "path", "type": "string", "description": "The file path to open."},
			{"name": "mode", "type": "string", "description": "The file mode ('r' for read, 'w' for write). Default is 'r'."}
		],
		"returns": [
			{"type": "stream", "description": "A stream object for reading/writing."}
		]})");
	return 1;
}

static int clua_io_open_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "open",
		"summary": "Open a file from the user's home path for reading or writing.",
		"parameters": [
			{"name": "path", "type": "string", "description": "The file path relative to the home path."},
			{"name": "mode", "type": "string", "description": "The file mode ('r' for read, 'w' for write). Default is 'r'."}
		],
		"returns": [
			{"type": "stream", "description": "A stream object for reading/writing."}
		]})");
	return 1;
}

void clua_ioregister(lua_State *s) {
	static const clua_Reg funcs[] = {
		{"sysopen", clua_io_sysopen, clua_io_sysopen_jsonhelp},
		{"open", clua_io_open, clua_io_open_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, clua_metaio(), "g_io");
}

void clua_register(lua_State *s) {
	clua_sysregister(s);
	clua_cmdregister(s);
	clua_varregister(s);
	clua_logregister(s);
	clua_ioregister(s);
}

const char *clua_metastream() {
	return "__global_stream";
}

const char *clua_metahttp() {
	return "__meta_http";
}

const char *clua_metacmd() {
	return "__meta_cmd";
}

const char *clua_metavar() {
	return "__meta_var";
}

const char *clua_metalog() {
	return "__meta_log";
}

const char *clua_metasys() {
	return "__meta_sys";
}

int clua_pushstream(lua_State* s, io::SeekableReadWriteStream *stream) {
	if (stream == nullptr) {
		return clua_error(s, "No stream given - can't push");
	}
	return clua_pushudata(s, stream, clua_metastream());
}

io::SeekableReadWriteStream *clua_tostream(lua_State* s, int n) {
	return *(io::SeekableReadWriteStream**)clua_getudata<io::SeekableReadWriteStream*>(s, n, clua_metastream());
}

bool clua_isstream(lua_State* s, int n) {
	return luaL_testudata(s, n, clua_metastream()) != nullptr;
}

image::Image *clua_toimage(lua_State* s, int n) {
	return *(image::Image**)clua_getudata<image::Image*>(s, n, clua_meta<image::Image>::name());
}

bool clua_isimage(lua_State* s, int n) {
	return luaL_testudata(s, n, clua_meta<image::Image>::name()) != nullptr;
}

int clua_pushimage(lua_State* s, image::Image *image) {
	if (image == nullptr) {
		return clua_error(s, "No image given - can't push");
	}
	return clua_pushudata(s, image, clua_meta<image::Image>::name());
}

static void clua_http_headers(lua_State *&s, int n, http::Request &request) {
	if (!lua_istable(s, n)) {
		return;
	}
	lua_pushnil(s);
	while (lua_next(s, n) != 0) {
		const char *key = luaL_checkstring(s, -2);
		const char *value = luaL_checkstring(s, -1);
		request.addHeader(key, value);
		lua_pop(s, 1);
	}
}

static int clua_http_requestexec(lua_State *s, http::Request &request) {
	io::BufferedReadWriteStream *outStream = new io::BufferedReadWriteStream(512);
	int status = 0;
	http::Headers outheaders;
	// TODO: this should be threaded and we should just return a future
	if (!request.execute(*outStream, &status, &outheaders)) {
		if (!outStream->empty()) {
			outStream->seek(0);
			core::String error;
			outStream->readString(outStream->size(), error, true);
			Log::error("%s", error.c_str());
		}
		delete outStream;
		return 0;
	}
	// TODO: this should get handled in the lua code
	if (!http::isValidStatusCode(status)) {
		if (!outStream->empty()) {
			outStream->seek(0);
			core::String error;
			outStream->readString(outStream->size(), error, true);
			Log::error("%s", error.c_str());
		}
		delete outStream;
		return 0;
	}
	outStream->seek(0);
	clua_pushstream(s, outStream);
	lua_newtable(s);
	for (const auto &it : outheaders) {
		lua_pushstring(s, it->first.c_str());
		lua_pushstring(s, it->second.c_str());
		lua_settable(s, -3);
	}
	return 2;
}

static int clua_http_get(lua_State *s) {
	const char *url = luaL_checkstring(s, 1);
	http::Request request(url, http::RequestType::GET);
	clua_http_headers(s, 2, request);
	const int ret = clua_http_requestexec(s, request);
	if (ret == 0) {
		const http::Headers &headers = request.headers();
		core::String headersStr;
		for (const auto &it : headers) {
			if (!headersStr.empty()) {
				headersStr += ", ";
			}
			headersStr += core::String::format("'%s: %s'", it->first.c_str(), it->second.c_str());
		}
		clua_error_prepare(s, "Failed to execute get request for url: %s (headers: %s)", request.url().c_str(),
						  headersStr.c_str());
		request.~Request();
		return lua_error(s);
	}
	return ret;
}

static int clua_http_get_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "get",
		"summary": "Perform an HTTP GET request.",
		"parameters": [
			{"name": "url", "type": "string", "description": "The URL to request."},
			{"name": "headers", "type": "table", "description": "Optional headers table."}
		],
		"returns": [
			{"type": "stream", "description": "Response body as stream."},
			{"type": "table", "description": "Response headers."}
		]})");
	return 1;
}

static int clua_stream_gc(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	delete stream;
	return 0;
}

static int clua_stream_readstring(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	const bool terminate = clua_optboolean(s, 2, false);
	core::String str;
	stream->readString(stream->size(), str, terminate);
	lua_pushstring(s, str.c_str());
	return 1;
}

static int clua_http_post(lua_State *s) {
	const char *url = luaL_checkstring(s, 1);
	http::Request request(url, http::RequestType::POST);
	const char *body = luaL_checkstring(s, 2);
	request.setBody(body);
	clua_http_headers(s, 3, request);
	const int ret = clua_http_requestexec(s, request);
	if (ret == 0) {
		const http::Headers &headers = request.headers();
		core::String headersStr;
		for (const auto &it : headers) {
			if (!headersStr.empty()) {
				headersStr += ", ";
			}
			headersStr += core::String::format("'%s: %s'", it->first.c_str(), it->second.c_str());
		}
		clua_error_prepare(s, "Failed to execute post request for url: %s and body '%s' (headers: %s)", request.url().c_str(),
						  request.body().c_str(), headersStr.c_str());
		request.~Request();
		return lua_error(s);
	}
	return ret;
}

static int clua_http_post_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "post",
		"summary": "Perform an HTTP POST request.",
		"parameters": [
			{"name": "url", "type": "string", "description": "The URL to request."},
			{"name": "body", "type": "string", "description": "The request body."},
			{"name": "headers", "type": "table", "description": "Optional headers table."}
		],
		"returns": [
			{"type": "stream", "description": "Response body as stream."},
			{"type": "table", "description": "Response headers."}
		]})");
	return 1;
}

void clua_httpregister(lua_State *s) {
	static const clua_Reg httpFuncs[] = {
		{"get", clua_http_get, clua_http_get_jsonhelp},
		{"post", clua_http_post, clua_http_post_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, httpFuncs, clua_metahttp(), "g_http");
}

static int clua_stream_readuint8(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint8_t v;
	if (stream->readUInt8(v) < 0) {
		return clua_error(s, "Failed to read uint8");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint8(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int8_t v;
	if (stream->readInt8(v) < 0) {
		return clua_error(s, "Failed to read int8");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readuint16(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint16_t v;
	if (stream->readUInt16(v) < 0) {
		return clua_error(s, "Failed to read uint16");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint16(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int16_t v;
	if (stream->readInt16(v) < 0) {
		return clua_error(s, "Failed to read int16");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readuint32(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint32_t v;
	if (stream->readUInt32(v) < 0) {
		return clua_error(s, "Failed to read uint32");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint32(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int32_t v;
	if (stream->readInt32(v) < 0) {
		return clua_error(s, "Failed to read int32");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readuint64(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint64_t v;
	if (stream->readUInt64(v) < 0) {
		return clua_error(s, "Failed to read uint64");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint64(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int64_t v;
	if (stream->readInt64(v) < 0) {
		return clua_error(s, "Failed to read int64");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readfloat(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	float v;
	if (stream->readFloat(v) < 0) {
		return clua_error(s, "Failed to read float");
	}
	lua_pushnumber(s, v);
	return 1;
}
static int clua_stream_readdouble(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	double v;
	if (stream->readDouble(v) < 0) {
		return clua_error(s, "Failed to read double");
	}
	lua_pushnumber(s, v);
	return 1;
}

static int clua_stream_writestring(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	const char *str = luaL_checkstring(s, 2);
	const bool terminate = clua_optboolean(s, 3, false);
	if (!stream->writeString(str, terminate)) {
		return clua_error(s, "Failed to write string");
	}
	return 0;
}

static int clua_stream_writeuint8(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint8_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt8(v)) {
		return clua_error(s, "Failed to write uint8");
	}
	return 0;
}

static int clua_stream_writeint8(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int8_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt8(v)) {
		return clua_error(s, "Failed to write int8");
	}
	return 0;
}

static int clua_stream_writeuint16(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint16_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt16(v)) {
		return clua_error(s, "Failed to write uint16");
	}
	return 0;
}

static int clua_stream_writeint16(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int16_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt16(v)) {
		return clua_error(s, "Failed to write int16");
	}
	return 0;
}

static int clua_stream_writeuint32(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint32_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt32(v)) {
		return clua_error(s, "Failed to write uint32");
	}
	return 0;
}

static int clua_stream_writeint32(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int32_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt32(v)) {
		return clua_error(s, "Failed to write int32");
	}
	return 0;
}

static int clua_stream_writeuint64(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint64_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt64(v)) {
		return clua_error(s, "Failed to write uint64");
	}
	return 0;
}

static int clua_stream_writeint64(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int64_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt64(v)) {
		return clua_error(s, "Failed to write int64");
	}
	return 0;
}

static int clua_stream_writefloat(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	float v = luaL_checknumber(s, 2);
	if (!stream->writeFloat(v)) {
		return clua_error(s, "Failed to write float");
	}
	return 0;
}

static int clua_stream_writedouble(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	double v = luaL_checknumber(s, 2);
	if (!stream->writeDouble(v)) {
		return clua_error(s, "Failed to write double");
	}
	return 0;
}

static int clua_stream_readuint16be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint16_t v;
	if (stream->readUInt16BE(v) < 0) {
		return clua_error(s, "Failed to read uint16");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint16be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int16_t v;
	if (stream->readInt16BE(v) < 0) {
		return clua_error(s, "Failed to read int16");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readuint32be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint32_t v;
	if (stream->readUInt32BE(v) < 0) {
		return clua_error(s, "Failed to read uint32");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint32be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int32_t v;
	if (stream->readInt32BE(v) < 0) {
		return clua_error(s, "Failed to read int32");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readuint64be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint64_t v;
	if (stream->readUInt64BE(v) < 0) {
		return clua_error(s, "Failed to read uint64");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readint64be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int64_t v;
	if (stream->readInt64BE(v) < 0) {
		return clua_error(s, "Failed to read int64");
	}
	lua_pushinteger(s, v);
	return 1;
}

static int clua_stream_readfloatbe(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	float v;
	if (stream->readFloatBE(v) < 0) {
		return clua_error(s, "Failed to read float");
	}
	lua_pushnumber(s, v);
	return 1;
}

static int clua_stream_readdoublebe(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	double v;
	if (stream->readDoubleBE(v) < 0) {
		return clua_error(s, "Failed to read double");
	}
	lua_pushnumber(s, v);
	return 1;
}

static int clua_stream_writeuint16be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint16_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt16BE(v)) {
		return clua_error(s, "Failed to write uint16");
	}
	return 0;
}

static int clua_stream_writeint16be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int16_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt16BE(v)) {
		return clua_error(s, "Failed to write int16");
	}
	return 0;
}

static int clua_stream_writeuint32be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint32_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt32BE(v)) {
		return clua_error(s, "Failed to write uint32");
	}
	return 0;
}

static int clua_stream_writeint32be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int32_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt32BE(v)) {
		return clua_error(s, "Failed to write int32");
	}
	return 0;
}

static int clua_stream_writeuint64be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	uint64_t v = luaL_checkinteger(s, 2);
	if (!stream->writeUInt64BE(v)) {
		return clua_error(s, "Failed to write uint64");
	}
	return 0;
}

static int clua_stream_writeint64be(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int64_t v = luaL_checkinteger(s, 2);
	if (!stream->writeInt64BE(v)) {
		return clua_error(s, "Failed to write int64");
	}
	return 0;
}

static int clua_stream_writefloatbe(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	float v = luaL_checknumber(s, 2);
	if (!stream->writeFloatBE(v)) {
		return clua_error(s, "Failed to write float");
	}
	return 0;
}

static int clua_stream_writedoublebe(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	double v = luaL_checknumber(s, 2);
	if (!stream->writeDoubleBE(v)) {
		return clua_error(s, "Failed to write double");
	}
	return 0;
}

static int clua_stream_writestream(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	io::SeekableReadWriteStream *other = clua_tostream(s, 2);
	if (!stream->writeStream(*other)) {
		return clua_error(s, "Failed to write stream");
	}
	return 0;
}

static int clua_stream_eos(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	lua_pushboolean(s, stream->eos());
	return 1;
}

static int clua_stream_seek(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	int64_t offset = luaL_checkinteger(s, 2);
	int mode = luaL_checkinteger(s, SEEK_SET);
	if (!stream->seek(offset, mode)) {
		return clua_error(s, "Failed to seek");
	}
	return 0;
}

static int clua_stream_tell(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	lua_pushinteger(s, stream->pos());
	return 1;
}

static int clua_stream_close(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	stream->close();
	return 0;
}

static int clua_stream_size(lua_State *s) {
	io::SeekableReadWriteStream *stream = clua_tostream(s, 1);
	lua_pushinteger(s, stream->size());
	return 1;
}

static int clua_image_gc(lua_State *s) {
	image::Image *image = clua_toimage(s, 1);
	delete image;
	return 0;
}

static int clua_image_name(lua_State *s) {
	image::Image *image = clua_toimage(s, 1);
	lua_pushstring(s, image->name().c_str());
	return 1;
}

static int clua_image_save(lua_State *s) {
	image::Image *image = clua_toimage(s, 1);
	const char *filename = luaL_checkstring(s, 2);
	const io::FilePtr &file = io::filesystem()->open(filename, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		return clua_error(s, "Failed to open file for saving: %s", filename);
	}
	io::FileStream stream(file);
	if (!image->writePNG(stream)) {
		return clua_error(s, "Failed to save image to %s", filename);
	}
	return 0;
}

static int clua_image_name_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "name",
		"summary": "Get the name of the image.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "The image name."}
		]})");
	return 1;
}

static int clua_image_save_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "save",
		"summary": "Save the image to a file.",
		"parameters": [
			{"name": "filename", "type": "string", "description": "The filename to save to."}
		],
		"returns": []})");
	return 1;
}

void clua_imageregister(lua_State *s) {
	static const clua_Reg imageFuncs[] = {
		{"name", clua_image_name, clua_image_name_jsonhelp},
		{"save", clua_image_save, clua_image_save_jsonhelp},
		{"__gc", clua_image_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, imageFuncs, clua_meta<image::Image>::name());
}

static int clua_stream_readstring_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readString",
		"summary": "Read a string from the stream.",
		"parameters": [
			{"name": "terminate", "type": "boolean", "description": "Whether to stop at null terminator (optional)."}
		],
		"returns": [
			{"type": "string", "description": "The string read."}
		]})");
	return 1;
}

static int clua_stream_readuint8_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt8",
		"summary": "Read an unsigned 8-bit integer from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint8_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt8",
		"summary": "Read a signed 8-bit integer from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readuint16_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt16",
		"summary": "Read an unsigned 16-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint16_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt16",
		"summary": "Read a signed 16-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readuint32_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt32",
		"summary": "Read an unsigned 32-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint32_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt32",
		"summary": "Read a signed 32-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readuint64_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt64",
		"summary": "Read an unsigned 64-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint64_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt64",
		"summary": "Read a signed 64-bit integer (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readfloat_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readFloat",
		"summary": "Read a 32-bit float (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "number", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readdouble_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readDouble",
		"summary": "Read a 64-bit double (little-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "number", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_writestring_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeString",
		"summary": "Write a string to the stream.",
		"parameters": [
			{"name": "str", "type": "string", "description": "The string to write."},
			{"name": "terminate", "type": "boolean", "description": "Whether to write null terminator (optional)."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint8_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt8",
		"summary": "Write an unsigned 8-bit integer to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint8_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt8",
		"summary": "Write a signed 8-bit integer to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint16_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt16",
		"summary": "Write an unsigned 16-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint16_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt16",
		"summary": "Write a signed 16-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint32_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt32",
		"summary": "Write an unsigned 32-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint32_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt32",
		"summary": "Write a signed 32-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint64_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt64",
		"summary": "Write an unsigned 64-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint64_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt64",
		"summary": "Write a signed 64-bit integer (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writefloat_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeFloat",
		"summary": "Write a 32-bit float (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "number", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writedouble_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeDouble",
		"summary": "Write a 64-bit double (little-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "number", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_readuint16be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt16BE",
		"summary": "Read an unsigned 16-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint16be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt16BE",
		"summary": "Read a signed 16-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readuint32be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt32BE",
		"summary": "Read an unsigned 32-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint32be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt32BE",
		"summary": "Read a signed 32-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readuint64be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readUInt64BE",
		"summary": "Read an unsigned 64-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readint64be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readInt64BE",
		"summary": "Read a signed 64-bit integer (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readfloatbe_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readFloatBE",
		"summary": "Read a 32-bit float (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "number", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_readdoublebe_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "readDoubleBE",
		"summary": "Read a 64-bit double (big-endian) from the stream.",
		"parameters": [],
		"returns": [
			{"type": "number", "description": "The value read."}
		]})");
	return 1;
}

static int clua_stream_writeuint16be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt16BE",
		"summary": "Write an unsigned 16-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint16be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt16BE",
		"summary": "Write a signed 16-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint32be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt32BE",
		"summary": "Write an unsigned 32-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint32be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt32BE",
		"summary": "Write a signed 32-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeuint64be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeUInt64BE",
		"summary": "Write an unsigned 64-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writeint64be_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeInt64BE",
		"summary": "Write a signed 64-bit integer (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "integer", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writefloatbe_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeFloatBE",
		"summary": "Write a 32-bit float (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "number", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writedoublebe_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeDoubleBE",
		"summary": "Write a 64-bit double (big-endian) to the stream.",
		"parameters": [
			{"name": "value", "type": "number", "description": "The value to write."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_writestream_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "writeStream",
		"summary": "Write the contents of another stream to this stream.",
		"parameters": [
			{"name": "source", "type": "stream", "description": "The source stream to read from."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_eos_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "eos",
		"summary": "Check if end of stream has been reached.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if at end of stream."}
		]})");
	return 1;
}

static int clua_stream_seek_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "seek",
		"summary": "Seek to a position in the stream.",
		"parameters": [
			{"name": "offset", "type": "integer", "description": "The offset to seek to."},
			{"name": "mode", "type": "integer", "description": "Seek mode (0=SET, 1=CUR, 2=END)."}
		],
		"returns": []})");
	return 1;
}

static int clua_stream_tell_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "tell",
		"summary": "Get the current position in the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The current position."}
		]})");
	return 1;
}

static int clua_stream_pos_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "pos",
		"summary": "Get the current position in the stream (alias for tell).",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The current position."}
		]})");
	return 1;
}

static int clua_stream_size_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "size",
		"summary": "Get the total size of the stream.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The stream size in bytes."}
		]})");
	return 1;
}

static int clua_stream_close_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "close",
		"summary": "Close the stream.",
		"parameters": [],
		"returns": []})");
	return 1;
}

void clua_streamregister(lua_State *s) {
	static const clua_Reg streamFuncs[] = {
		{"readString", clua_stream_readstring, clua_stream_readstring_jsonhelp},
		{"readUInt8", clua_stream_readuint8, clua_stream_readuint8_jsonhelp},
		{"readInt8", clua_stream_readint8, clua_stream_readint8_jsonhelp},
		{"readUInt16", clua_stream_readuint16, clua_stream_readuint16_jsonhelp},
		{"readInt16", clua_stream_readint16, clua_stream_readint16_jsonhelp},
		{"readUInt32", clua_stream_readuint32, clua_stream_readuint32_jsonhelp},
		{"readInt32", clua_stream_readint32, clua_stream_readint32_jsonhelp},
		{"readUInt64", clua_stream_readuint64, clua_stream_readuint64_jsonhelp},
		{"readInt64", clua_stream_readint64, clua_stream_readint64_jsonhelp},
		{"readFloat", clua_stream_readfloat, clua_stream_readfloat_jsonhelp},
		{"readDouble", clua_stream_readdouble, clua_stream_readdouble_jsonhelp},
		{"writeString", clua_stream_writestring, clua_stream_writestring_jsonhelp},
		{"writeUInt8", clua_stream_writeuint8, clua_stream_writeuint8_jsonhelp},
		{"writeInt8", clua_stream_writeint8, clua_stream_writeint8_jsonhelp},
		{"writeUInt16", clua_stream_writeuint16, clua_stream_writeuint16_jsonhelp},
		{"writeInt16", clua_stream_writeint16, clua_stream_writeint16_jsonhelp},
		{"writeUInt32", clua_stream_writeuint32, clua_stream_writeuint32_jsonhelp},
		{"writeInt32", clua_stream_writeint32, clua_stream_writeint32_jsonhelp},
		{"writeUInt64", clua_stream_writeuint64, clua_stream_writeuint64_jsonhelp},
		{"writeInt64", clua_stream_writeint64, clua_stream_writeint64_jsonhelp},
		{"writeFloat", clua_stream_writefloat, clua_stream_writefloat_jsonhelp},
		{"writeDouble", clua_stream_writedouble, clua_stream_writedouble_jsonhelp},
		{"readUInt16BE", clua_stream_readuint16be, clua_stream_readuint16be_jsonhelp},
		{"readInt16BE", clua_stream_readint16be, clua_stream_readint16be_jsonhelp},
		{"readUInt32BE", clua_stream_readuint32be, clua_stream_readuint32be_jsonhelp},
		{"readInt32BE", clua_stream_readint32be, clua_stream_readint32be_jsonhelp},
		{"readUInt64BE", clua_stream_readuint64be, clua_stream_readuint64be_jsonhelp},
		{"readInt64BE", clua_stream_readint64be, clua_stream_readint64be_jsonhelp},
		{"readFloatBE", clua_stream_readfloatbe, clua_stream_readfloatbe_jsonhelp},
		{"readDoubleBE", clua_stream_readdoublebe, clua_stream_readdoublebe_jsonhelp},
		{"writeUInt16BE", clua_stream_writeuint16be, clua_stream_writeuint16be_jsonhelp},
		{"writeInt16BE", clua_stream_writeint16be, clua_stream_writeint16be_jsonhelp},
		{"writeUInt32BE", clua_stream_writeuint32be, clua_stream_writeuint32be_jsonhelp},
		{"writeInt32BE", clua_stream_writeint32be, clua_stream_writeint32be_jsonhelp},
		{"writeUInt64BE", clua_stream_writeuint64be, clua_stream_writeuint64be_jsonhelp},
		{"writeInt64BE", clua_stream_writeint64be, clua_stream_writeint64be_jsonhelp},
		{"writeFloatBE", clua_stream_writefloatbe, clua_stream_writefloatbe_jsonhelp},
		{"writeDoubleBE", clua_stream_writedoublebe, clua_stream_writedoublebe_jsonhelp},
		{"writeStream", clua_stream_writestream, clua_stream_writestream_jsonhelp},
		{"eos", clua_stream_eos, clua_stream_eos_jsonhelp},
		{"seek", clua_stream_seek, clua_stream_seek_jsonhelp},
		{"tell", clua_stream_tell, clua_stream_tell_jsonhelp},
		{"pos", clua_stream_tell, clua_stream_pos_jsonhelp},
		{"size", clua_stream_size, clua_stream_size_jsonhelp},
		{"close", clua_stream_close, clua_stream_close_jsonhelp},
		{"__gc", clua_stream_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, streamFuncs, clua_metastream());
}

void clua_mathregister(lua_State *s) {
	clua_vecregister<glm::vec2>(s);
	clua_vecregister<glm::vec3>(s);
	clua_vecregister<glm::vec4>(s);
	clua_vecregister<glm::ivec2>(s);
	clua_vecregister<glm::ivec3>(s);
	clua_vecregister<glm::ivec4>(s);
	clua_quatregister(s);
}
