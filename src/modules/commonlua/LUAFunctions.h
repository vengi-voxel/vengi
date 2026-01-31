/**
 * @file
 */
#pragma once

#include "commonlua/LUA.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include <lauxlib.h>
#include <lua.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

namespace io {
class SeekableReadWriteStream;
}

void clua_assert(lua_State* s, bool pass, const char *msg);
void clua_assert_argc(lua_State* s, bool pass);

template<class T>
struct clua_meta {};

template<> struct clua_meta<glm::bvec2> { static char const *name() {return "__meta_bvec2";} };
template<> struct clua_meta<glm::bvec3> { static char const *name() {return "__meta_bvec3";} };
template<> struct clua_meta<glm::bvec4> { static char const *name() {return "__meta_bvec4";} };
template<> struct clua_meta<glm::dvec2> { static char const *name() {return "__meta_dvec2";} };
template<> struct clua_meta<glm::dvec3> { static char const *name() {return "__meta_dvec3";} };
template<> struct clua_meta<glm::dvec4> { static char const *name() {return "__meta_dvec4";} };
template<> struct clua_meta<glm::ivec2> { static char const *name() {return "__meta_ivec2";} };
template<> struct clua_meta<glm::ivec3> { static char const *name() {return "__meta_ivec3";} };
template<> struct clua_meta<glm::ivec4> { static char const *name() {return "__meta_ivec4";} };
template<> struct clua_meta<glm::vec2> { static char const *name() {return "__meta_vec2";} };
template<> struct clua_meta<glm::vec3> { static char const *name() {return "__meta_vec3";} };
template<> struct clua_meta<glm::vec4> { static char const *name() {return "__meta_vec4";} };
template<> struct clua_meta<glm::quat> { static char const *name() {return "__meta_quat";} };
template<> struct clua_meta<image::Image> { static char const *name() {return "__meta_image";} };

template<class T>
struct clua_name {};

template<> struct clua_name<glm::bvec2> { static char const *name() {return "g_bvec2";} };
template<> struct clua_name<glm::bvec3> { static char const *name() {return "g_bvec3";} };
template<> struct clua_name<glm::bvec4> { static char const *name() {return "g_bvec4";} };
template<> struct clua_name<glm::dvec2> { static char const *name() {return "g_dvec2";} };
template<> struct clua_name<glm::dvec3> { static char const *name() {return "g_dvec3";} };
template<> struct clua_name<glm::dvec4> { static char const *name() {return "g_dvec4";} };
template<> struct clua_name<glm::ivec2> { static char const *name() {return "g_ivec2";} };
template<> struct clua_name<glm::ivec3> { static char const *name() {return "g_ivec3";} };
template<> struct clua_name<glm::ivec4> { static char const *name() {return "g_ivec4";} };
template<> struct clua_name<glm::vec2> { static char const *name() {return "g_vec2";} };
template<> struct clua_name<glm::vec3> { static char const *name() {return "g_vec3";} };
template<> struct clua_name<glm::vec4> { static char const *name() {return "g_vec4";} };
template<> struct clua_name<glm::quat> { static char const *name() {return "g_quat";} };

int clua_assignmetatable(lua_State* s, const char *name);

int clua_ioloader(lua_State *s);

template<class T>
T* clua_newuserdata(lua_State* s, const T& data) {
	T* udata = (T*) lua_newuserdata(s, sizeof(T));
	*udata = data;
	return udata;
}

template<class T>
int clua_pushudata(lua_State* s, const T& data, const char *name) {
	clua_newuserdata<T>(s, data);
	return clua_assignmetatable(s, name);
}

template<class T>
inline T clua_getudata(lua_State* s, int n, const char *name) {
	return (T) luaL_checkudata(s, n, name);
}

int clua_error(lua_State *s, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(2);

// call lua_error afterwards - allows you to call destructors or do cleanup
void clua_error_prepare(lua_State *s, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(2);


template<class T>
bool clua_istype(lua_State *s, int n) {
	using RAWTYPE = typename core::remove_pointer<T>::type;
	return luaL_testudata(s, n, clua_meta<RAWTYPE>::name()) != nullptr;
}

template<typename T>
bool clua_isvec(lua_State *s, int n) {
	using RAWTYPE = typename core::remove_pointer<T>::type;
	if (!lua_istable(s, n)) {
		return false;
	}
	if (lua_getmetatable(s, n) == 0) {
		return false;
	}
	luaL_getmetatable(s, clua_meta<RAWTYPE>::name());
	if (lua_rawequal(s, -1, -2)) {
		lua_pop(s, 2);
		return true;
	}
	lua_pop(s, 2);
	return false;
}

template<class T>
int clua_push(lua_State* s, const T& v) {
	using RAWTYPE = typename core::remove_pointer<T>::type;
	return clua_pushudata<T>(s, v, clua_meta<RAWTYPE>::name());
}

template<class T>
T* clua_get(lua_State *s, int n) {
	using RAWTYPE = typename core::remove_pointer<T>::type;
	return clua_getudata<T*>(s, n, clua_meta<RAWTYPE>::name());
}

bool clua_registernew(lua_State* s, const char *name, lua_CFunction func);
bool clua_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name);
bool clua_registerfuncsglobal(lua_State* s, const luaL_Reg* funcs, const char *meta, const char *name);

struct clua_Reg {
  const char *name;
  lua_CFunction func;
  lua_CFunction jsonHelp;
};
bool clua_registerfuncs(lua_State* s, const clua_Reg* funcs, const char *name);
bool clua_registerfuncsglobal(lua_State* s, const clua_Reg* funcs, const char *meta, const char *name);

/**
 * @brief Get the jsonhelp function for a method in a metatable
 * @param s The lua state
 * @param name The metatable name
 * @param method The method name
 * @return The jsonhelp function or nullptr if not found
 */
lua_CFunction clua_getjsonhelp(lua_State* s, const char *name, const char *method);

template<class T>
struct LuaNumberFuncs {};

template<>
struct LuaNumberFuncs<float> {
	static void push(lua_State *s, float n) {
		lua_pushnumber(s, n);
	}

	static float check(lua_State *s, int arg) {
		return luaL_checknumber(s, arg);
	}
};

template<>
struct LuaNumberFuncs<double> {
	static void push(lua_State *s, double n) {
		lua_pushnumber(s, n);
	}

	static double check(lua_State *s, int arg) {
		return luaL_checknumber(s, arg);
	}
};

template<>
struct LuaNumberFuncs<int> {
	static void push(lua_State *s, int n) {
		lua_pushinteger(s, n);
	}

	static int check(lua_State *s, int arg) {
		return (int)luaL_checkinteger(s, arg);
	}
};

template<>
struct LuaNumberFuncs<bool> {
	static void push(lua_State *s, bool n) {
		lua_pushinteger(s, n);
	}

	static bool check(lua_State *s, int arg) {
		return (int)luaL_checkinteger(s, arg) != 0;
	}
};

static constexpr const char *VEC_MEMBERS[] = { "x", "y", "z", "w" };

template<>
inline int clua_push<glm::quat>(lua_State* s, const glm::quat& v) {
	lua_newtable(s);

	for (int i = 0; i < 4; ++i) {
		lua_pushstring(s, VEC_MEMBERS[i]);
		LuaNumberFuncs<glm::quat::value_type>::push(s, v[i]);
		lua_settable(s, -3);
	}

	luaL_getmetatable(s, clua_meta<glm::quat>::name());
	lua_setmetatable(s, -2);
	return 1;
}

template<>
inline int clua_push<float>(lua_State* s, const float& v) {
	lua_pushnumber(s, v);
	return 1;
}

template<>
inline int clua_push<int>(lua_State* s, const int& v) {
	lua_pushinteger(s, v);
	return 1;
}

glm::quat clua_toquat(lua_State *s, int n);
bool clua_isquat(lua_State *s, int n);

template<class T>
T clua_tovec(lua_State *s, int n) {
	luaL_checktype(s, n, LUA_TTABLE);
	T v{0};
	for (int i = 0; i < T::length(); ++i) {
		lua_getfield(s, n, VEC_MEMBERS[i]);
		v[i] = LuaNumberFuncs<typename T::value_type>::check(s, -1);
		lua_pop(s, 1);
	}
	return v;
}

template<int N, typename T>
int clua_push(lua_State* s, const glm::vec<N, T>& v) {
	using RAWTYPE = glm::vec<N, T>;
	lua_newtable(s);

	for (int i = 0; i < N; ++i) {
		lua_pushstring(s, VEC_MEMBERS[i]);
		LuaNumberFuncs<T>::push(s, v[i]);
		lua_settable(s, -3);
	}

	luaL_getmetatable(s, clua_meta<RAWTYPE>::name());
	lua_setmetatable(s, -2);
	return 1;
}

template<class T>
int clua_vecadd(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a + b;
		return clua_push(s, c);
	}
	const T& c = a + (typename T::value_type)lua_tonumber(s, 2);
	return clua_push(s, c);
}

template<class T>
int clua_vecdiv(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a / b;
		return clua_push(s, c);
	}
	const T& c = a / (typename T::value_type)lua_tonumber(s, 2);
	return clua_push(s, c);
}

template<class T>
int clua_vecmul(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a * b;
		return clua_push(s, c);
	}
	const T& c = a * (typename T::value_type)lua_tonumber(s, 2);
	return clua_push(s, c);
}

template<class T>
struct clua_vecequal {};

template<int N>
struct clua_vecequal<glm::vec<N, float> > {
static int equal(lua_State* s) {
	const glm::vec<N, float>& a = clua_tovec<glm::vec<N, float> >(s, 1);
	const glm::vec<N, float>& b = clua_tovec<glm::vec<N, float> >(s, 2);
	const bool e = glm::all(glm::epsilonEqual(a, b, 0.0001f));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, double> > {
static int equal(lua_State* s) {
	const glm::vec<N, double>& a = clua_tovec<glm::vec<N, double> >(s, 1);
	const glm::vec<N, double>& b = clua_tovec<glm::vec<N, double> >(s, 2);
	const bool e = glm::all(glm::epsilonEqual(a, b, 0.0001));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, int> > {
static int equal(lua_State* s) {
	const glm::vec<N, int>& a = clua_tovec<glm::vec<N, int> >(s, 1);
	const glm::vec<N, int>& b = clua_tovec<glm::vec<N, int> >(s, 2);
	const bool e = glm::all(glm::equal(a, b));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, bool> > {
static int equal(lua_State* s) {
	const glm::vec<N, bool>& a = clua_tovec<glm::vec<N, bool> >(s, 1);
	const glm::vec<N, bool>& b = clua_tovec<glm::vec<N, bool> >(s, 2);
	const bool e = glm::all(glm::equal(a, b));
	lua_pushboolean(s, e);
	return 1;
}
};

template<class T>
int clua_vecsub(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	const T& b = clua_tovec<T>(s, 2);
	const T& c = a - b;
	clua_push(s, c);
	return 1;
}

template<class T>
int clua_vecnegate(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	clua_push<T>(s, -a);
	return 1;
}

template<class T>
int clua_vectostring(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	core::String str;
	for (int i = 0; i < T::length(); ++i) {
		if (i != 0) {
			str.append(":");
		}
		str.append(core::string::toString(a[i]));
	}
	lua_pushfstring(s, "%s", str.c_str());
	return 1;
}

template<class T>
struct clua_vecnew {};

template<int N>
struct clua_vecnew<glm::vec<N, float> > {
static int exec(lua_State* s) {
	glm::vec<N, float> array;
	float value = 0.0f;
	for (int i = 0; i < N; ++i) {
		array[i] = luaL_optnumber(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, double> > {
static int exec(lua_State* s) {
	glm::vec<N, double> array;
	float value = 0.0f;
	for (int i = 0; i < N; ++i) {
		array[i] = luaL_optnumber(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, int> > {
static int exec(lua_State* s) {
	glm::vec<N, int> array;
	int value = 0;
	for (int i = 0; i < N; ++i) {
		array[i] = luaL_optinteger(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, bool> > {
static int exec(lua_State* s) {
	glm::vec<N, bool> array;
	int value = 0;
	for (int i = 0; i < N; ++i) {
		array[i] = luaL_optinteger(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<class T>
struct clua_vecfunc {};

#define LUA_GLM_VEC_FUNC2(type, func) \
	static int func(lua_State* s) { \
		const glm::vec<N, type> v1 = clua_tovec<glm::vec<N, type> >(s, 1); \
		const glm::vec<N, type> v2 = clua_tovec<glm::vec<N, type> >(s, 2); \
		return clua_push(s, glm::func(v1, v2)); \
	}

#define LUA_GLM_VEC_FUNC2_unsupported(type, func) \
	static int func(lua_State* s) { \
		return clua_error(s, CORE_STRINGIFY(func) " is not supported for vector of type " CORE_STRINGIFY(type)); \
	}

#define LUA_GLM_VEC_FUNC1(type, func) \
	static int func(lua_State* s) { \
		if (lua_isnumber(s, 1)) { \
			glm::vec<N, type> v1; \
			for (int i = 0; i < N; ++i) { \
				v1[i] = lua_tonumber(s, i + 1); \
			} \
			return clua_push(s, glm::func(v1)); \
		} \
		const glm::vec<N, type> v1 = clua_tovec<glm::vec<N, type> >(s, 1); \
		return clua_push(s, glm::func(v1)); \
	}

#define LUA_GLM_VEC_FUNC1_unsupported(type, func) \
	static int func(lua_State* s) { \
		return clua_error(s, CORE_STRINGIFY(func) " is not supported for vector of type " CORE_STRINGIFY(type)); \
	}

template<int N>
struct clua_vecfunc<glm::vec<N, float> > {
	LUA_GLM_VEC_FUNC1(float, normalize)
	LUA_GLM_VEC_FUNC1(float, length)
	LUA_GLM_VEC_FUNC2(float, distance)
	LUA_GLM_VEC_FUNC2(float, dot)
};

template<int N>
struct clua_vecfunc<glm::vec<N, double> > {
	LUA_GLM_VEC_FUNC1(double, normalize)
	LUA_GLM_VEC_FUNC1(double, length)
	LUA_GLM_VEC_FUNC2(double, distance)
	LUA_GLM_VEC_FUNC2(double, dot)
};

template<int N>
struct clua_vecfunc<glm::vec<N, bool> > {
	LUA_GLM_VEC_FUNC1_unsupported(bool, normalize)
	LUA_GLM_VEC_FUNC1_unsupported(bool, length)
	LUA_GLM_VEC_FUNC2_unsupported(bool, distance)
	LUA_GLM_VEC_FUNC2_unsupported(bool, dot)
};

template<int N>
struct clua_vecfunc<glm::vec<N, int> > {
	LUA_GLM_VEC_FUNC1_unsupported(int, normalize)
	LUA_GLM_VEC_FUNC1_unsupported(int, length)
	LUA_GLM_VEC_FUNC2_unsupported(int, distance)
	LUA_GLM_VEC_FUNC2_unsupported(int, dot)
};

template<class T>
static int clua_vecindex(lua_State *s) {
	const T* v = clua_get<T>(s, 1);
	const char* i = luaL_checkstring(s, 2);

	switch (*i) {
	case '0':
	case 'x':
	case 'r':
		LuaNumberFuncs<typename T::value_type>::push(s, (*v)[0]);
		return 1;
	case '1':
	case 'y':
	case 'g':
		if (T::length() >= 2) {
			LuaNumberFuncs<typename T::value_type>::push(s, (*v)[1]);
			return 1;
		}
		break;
	case '2':
	case 'z':
	case 'b':
		if (T::length() >= 3) {
			LuaNumberFuncs<typename T::value_type>::push(s, (*v)[2]);
			return 1;
		}
		break;
	case '3':
	case 'w':
	case 'a':
		if (T::length() >= 4) {
			LuaNumberFuncs<typename T::value_type>::push(s, (*v)[3]);
			return 1;
		}
		break;
	default:
		break;
	}
	return clua_error(s, "Invalid component %c", *i);
}

template<class T>
static int clua_vecnewindex(lua_State *s) {
	T* v = clua_get<T>(s, 1);
	const char *i = luaL_checkstring(s, 2);
	const typename T::value_type t = LuaNumberFuncs<typename T::value_type>::check(s, 3);

	switch (*i) {
	case '0':
	case 'x':
	case 'r':
		(*v)[0] = t;
		return 0;
	case '1':
	case 'y':
	case 'g':
		if (T::length() >= 2) {
			(*v)[1] = t;
			return 0;
		}
		break;
	case '2':
	case 'z':
	case 'b':
		if (T::length() >= 3) {
			(*v)[2] = t;
			return 0;
		}
		break;
	case '3':
	case 'w':
	case 'a':
		if (T::length() >= 4) {
			(*v)[3] = t;
			return 0;
		}
		break;
	default:
		break;
	}
	return clua_error(s, "Invalid component %c", *i);
}

// Vector jsonhelp functions (generic for all vector types)
inline int clua_vec_new_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "new",
		"summary": "Create a new vector with the specified components.",
		"parameters": [
			{"name": "x", "type": "number", "description": "The X component."},
			{"name": "y", "type": "number", "description": "The Y component (for vec2 and higher)."},
			{"name": "z", "type": "number", "description": "The Z component (for vec3 and higher)."},
			{"name": "w", "type": "number", "description": "The W component (for vec4 only)."}
		],
		"returns": [
			{"type": "vec", "description": "A new vector with the specified components."}
		]})");
	return 1;
}

inline int clua_vec_distance_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "distance",
		"summary": "Calculate the distance between two vectors.",
		"parameters": [
			{"name": "a", "type": "vec", "description": "The first vector."},
			{"name": "b", "type": "vec", "description": "The second vector."}
		],
		"returns": [
			{"type": "number", "description": "The Euclidean distance between the two vectors."}
		]})");
	return 1;
}

inline int clua_vec_dot_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "dot",
		"summary": "Calculate the dot product of two vectors.",
		"parameters": [
			{"name": "a", "type": "vec", "description": "The first vector."},
			{"name": "b", "type": "vec", "description": "The second vector."}
		],
		"returns": [
			{"type": "number", "description": "The dot product of the two vectors."}
		]})");
	return 1;
}

inline int clua_vec_length_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "length",
		"summary": "Calculate the length (magnitude) of a vector.",
		"parameters": [
			{"name": "v", "type": "vec", "description": "The vector."}
		],
		"returns": [
			{"type": "number", "description": "The length of the vector."}
		]})");
	return 1;
}

inline int clua_vec_normalize_jsonhelp(lua_State *s) {
	lua_pushstring(s, R"({
		"name": "normalize",
		"summary": "Normalize a vector to unit length.",
		"parameters": [
			{"name": "v", "type": "vec", "description": "The vector to normalize."}
		],
		"returns": [
			{"type": "vec", "description": "The normalized vector with length 1."}
		]})");
	return 1;
}

template<class T>
void clua_vecregister(lua_State* s) {
	using RAWTYPE = typename core::remove_reference<typename core::remove_pointer<T>::type>::type;
	const luaL_Reg funcs[] = {
		{"__add", clua_vecadd<RAWTYPE>},
		{"__sub", clua_vecsub<RAWTYPE>},
		{"__mul", clua_vecmul<RAWTYPE>},
		{"__div", clua_vecdiv<RAWTYPE>},
		{"__unm", clua_vecnegate<RAWTYPE>},
		{"__len", clua_vecfunc<RAWTYPE>::length},
		{"__eq", clua_vecequal<RAWTYPE>::equal},
		{"__tostring", clua_vectostring<RAWTYPE>},
		{"__index", clua_vecindex<RAWTYPE>},
		{"__newindex", clua_vecnewindex<RAWTYPE>},
		// TODO: __pow, __mod, __idiv, __lt, __le
		{"distance", clua_vecfunc<RAWTYPE>::distance},
		{"dot", clua_vecfunc<RAWTYPE>::dot},
		{"length", clua_vecfunc<RAWTYPE>::length},
		{"normalize", clua_vecfunc<RAWTYPE>::normalize},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<RAWTYPE>::name());
	clua_registerfuncs(s, funcs, clua_meta<RAWTYPE>::name());
	static const clua_Reg globalFuncs[] = {
		{"new", clua_vecnew<RAWTYPE>::exec, clua_vec_new_jsonhelp},
		{"distance", clua_vecfunc<RAWTYPE>::distance, clua_vec_distance_jsonhelp},
		{"dot", clua_vecfunc<RAWTYPE>::dot, clua_vec_dot_jsonhelp},
		{"length", clua_vecfunc<RAWTYPE>::length, clua_vec_length_jsonhelp},
		{"normalize", clua_vecfunc<RAWTYPE>::normalize, clua_vec_normalize_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	const core::String& globalMeta = core::String::format("%s_global", clua_meta<RAWTYPE>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<RAWTYPE>::name());
}

int clua_errorhandler(lua_State* s);
void clua_quatregister(lua_State* s);

bool clua_optboolean(lua_State* s, int index, bool defaultVal);

int clua_typerror(lua_State *s, int narg, const char *tname);

int clua_checkboolean(lua_State *s, int index);

int clua_pushstream(lua_State* s, io::SeekableReadWriteStream *stream);
io::SeekableReadWriteStream *clua_tostream(lua_State* s, int n);
bool clua_isstream(lua_State* s, int n);

image::Image *clua_toimage(lua_State* s, int n);
bool clua_isimage(lua_State* s, int n);
// the image is released by lua
int clua_pushimage(lua_State* s, image::Image *image);

/**
 * @brief Registers all shared lua modules/globals/functions
 */
void clua_register(lua_State *s);
void clua_mathregister(lua_State *s);
// needs clua_streamregister
void clua_httpregister(lua_State *s);
void clua_streamregister(lua_State *s);

void clua_imageregister(lua_State *s);
void clua_ioregister(lua_State *s);
void clua_cmdregister(lua_State *s);
void clua_varregister(lua_State *s);
void clua_logregister(lua_State *s);

const char *clua_metahttp();
const char *clua_metaio();
const char *clua_metastream();
const char *clua_metacmd();
const char *clua_metavar();
const char *clua_metalog();
const char *clua_metasys();
