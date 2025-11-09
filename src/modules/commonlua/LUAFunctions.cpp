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

void clua_cmdregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"execute", clua_cmdexecute},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metacmd", "g_cmd");
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

void clua_varregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"create", clua_varcreate},
		{"str", clua_vargetstr},
		{"bool", clua_vargetbool},
		{"int", clua_vargetint},
		{"float", clua_vargetfloat},
		{"setStr", clua_varsetstr},
		{"setBool", clua_varsetbool},
		{"setInt", clua_varsetint},
		{"setFloat", clua_varsetfloat},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metavar", "g_var");
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
	clua_registerfuncsglobal(s, funcs, "_metalog", "g_log");
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

static constexpr glm::quat rotateXZ(float angleX, float angleZ) {
	return glm::quat(glm::vec3(angleX, 0.0f, angleZ));
}

static constexpr glm::quat rotateXY(float angleX, float angleY) {
	return glm::quat(glm::vec3(angleX, angleY, 0.0f));
}

static int clua_quat_rotate_xyz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXZ(x, z));
}

static int clua_quat_rotate_xy(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float y = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, y));
}

static int clua_quat_rotate_yz(lua_State* s) {
	const float y = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(y, z));
}

static int clua_quat_rotate_xz(lua_State* s) {
	const float x = lua_tonumber(s, 1);
	const float z = lua_tonumber(s, 2);
	return clua_push(s, rotateXY(x, z));
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

void clua_quatregister(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"__mul", clua_quatmul},
		{"__index", clua_quatindex},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<glm::quat>::name());
	clua_registerfuncs(s, funcs, clua_meta<glm::quat>::name());
	const luaL_Reg globalFuncs[] = {
		{"new",          clua_quat_new},
		{"rotateXYZ",    clua_quat_rotate_xyz},
		{"rotateXY",     clua_quat_rotate_xy},
		{"rotateYZ",     clua_quat_rotate_yz},
		{"rotateXZ",     clua_quat_rotate_xz},
		{"rotateX",      clua_quat_rotate_x},
		{"rotateY",      clua_quat_rotate_y},
		{"rotateZ",      clua_quat_rotate_z},
		{nullptr, nullptr}
	};
	const core::String& globalMeta = core::String::format("%s_global", clua_meta<glm::quat>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<glm::quat>::name());
}

#define ISSUE_602 0
#if ISSUE_602
struct SleepState {
	uint64_t wakeupTime;
};

static int clua_yield_sleep(lua_State *s) {
	SleepState *sleepState = (SleepState *)lua_touserdata(s, lua_upvalueindex(1));
	app::App *app = app::App::getInstance();
	const core::TimeProviderPtr &timeProvider = app->timeProvider();

	if (timeProvider->systemMillis() >= sleepState->wakeupTime) {
		// Time is up, resume the coroutine
		return 0;
	}

	// Not yet ready, re-yield
	return lua_yield(s, 0);
}
#endif

static int clua_syssleep(lua_State *s) {
	const int ms = luaL_checkinteger(s, 1);
	app::App* app = app::App::getInstance();
	if (!lua_isyieldable(s)) {
		app->wait(ms);
		return 0;
	}
#if ISSUE_602
	// TODO: implement me: https://github.com/vengi-voxel/vengi/issues/602
	const core::TimeProviderPtr &timeProvider = app->timeProvider();
	const uint64_t currentMillis = timeProvider->systemMillis();

	if (ms > 0) {
		// Calculate when the coroutine should wake up
		SleepState *sleepState = (SleepState *)lua_newuserdata(s, sizeof(SleepState));
		sleepState->wakeupTime = currentMillis + ms;

		// Register a continuation for this coroutine with the userdata as an upvalue
		lua_pushcclosure(s, clua_yield_sleep, 1);  // Set 1 upvalue (userdata)

		return lua_yield(s, 0);
	}
#endif
	app->wait(ms);
	return 0;
}

static int clua_sysshouldquit(lua_State *s) {
	app::App* app = app::App::getInstance();
	lua_pushboolean(s, app->shouldQuit());
	return 1;
}

static void clua_sysregister(lua_State *s) {
	const luaL_Reg funcs[] = {
		{"sleep", clua_syssleep},
		{"shouldQuit", clua_sysshouldquit},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metasys", "g_sys");
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

void clua_ioregister(lua_State *s) {
	static const luaL_Reg funcs[] = {
		{"sysopen", clua_io_sysopen},
		{"open", clua_io_open},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metaio", "g_io");
}

void clua_register(lua_State *s) {
	clua_sysregister(s);
	clua_cmdregister(s);
	clua_varregister(s);
	clua_logregister(s);
	clua_ioregister(s);
}

static const char *clua_metastream() {
	return "__global_stream";
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

void clua_httpregister(lua_State *s) {
	static const luaL_Reg shapeFuncs[] = {
		{"get", clua_http_get},
		{"post", clua_http_post},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, shapeFuncs, "__meta_http", "g_http");
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

void clua_imageregister(lua_State *s) {
	static const luaL_Reg imageFuncs[] = {
		{"name", clua_image_name},
		{"save", clua_image_save},
		{"__gc", clua_image_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, imageFuncs, clua_meta<image::Image>::name());
}

void clua_streamregister(lua_State *s) {
	static const luaL_Reg streamFuncs[] = {
		{"readString", clua_stream_readstring},
		{"readUInt8", clua_stream_readuint8},
		{"readInt8", clua_stream_readint8},
		{"readUInt16", clua_stream_readuint16},
		{"readInt16", clua_stream_readint16},
		{"readUInt32", clua_stream_readuint32},
		{"readInt32", clua_stream_readint32},
		{"readUInt64", clua_stream_readuint64},
		{"readInt64", clua_stream_readint64},
		{"readFloat", clua_stream_readfloat},
		{"readDouble", clua_stream_readdouble},
		{"writeString", clua_stream_writestring},
		{"writeUInt8", clua_stream_writeuint8},
		{"writeInt8", clua_stream_writeint8},
		{"writeUInt16", clua_stream_writeuint16},
		{"writeInt16", clua_stream_writeint16},
		{"writeUInt32", clua_stream_writeuint32},
		{"writeInt32", clua_stream_writeint32},
		{"writeUInt64", clua_stream_writeuint64},
		{"writeInt64", clua_stream_writeint64},
		{"writeFloat", clua_stream_writefloat},
		{"writeDouble", clua_stream_writedouble},
		{"readUInt16BE", clua_stream_readuint16be},
		{"readInt16BE", clua_stream_readint16be},
		{"readUInt32BE", clua_stream_readuint32be},
		{"readInt32BE", clua_stream_readint32be},
		{"readUInt64BE", clua_stream_readuint64be},
		{"readInt64BE", clua_stream_readint64be},
		{"readFloatBE", clua_stream_readfloatbe},
		{"readDoubleBE", clua_stream_readdoublebe},
		{"writeUInt16BE", clua_stream_writeuint16be},
		{"writeInt16BE", clua_stream_writeint16be},
		{"writeUInt32BE", clua_stream_writeuint32be},
		{"writeInt32BE", clua_stream_writeint32be},
		{"writeUInt64BE", clua_stream_writeuint64be},
		{"writeInt64BE", clua_stream_writeint64be},
		{"writeFloatBE", clua_stream_writefloatbe},
		{"writeDoubleBE", clua_stream_writedoublebe},
		{"writeStream", clua_stream_writestream},
		{"eos", clua_stream_eos},
		{"seek", clua_stream_seek},
		{"tell", clua_stream_tell},
		{"pos", clua_stream_tell},
		{"size", clua_stream_size},
		{"close", clua_stream_close},
		{"__gc", clua_stream_gc},
		{nullptr, nullptr}
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
