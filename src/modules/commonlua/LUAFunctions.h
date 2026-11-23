/**
 * @file
 */
#pragma once

#include "commonlua/LUA.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include <lauxlib.h>
#include <lua.h>
#include "math/Quatf.h"
#include "math/Vec4i.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"

namespace io {
class SeekableReadWriteStream;
class WriteStream;
}

void clua_assert(lua_State* s, bool pass, const char *msg);
void clua_assert_argc(lua_State* s, bool pass);

template<class T>
struct clua_meta {};

template<> struct clua_meta<math::BVec2> { static char const *name() {return "__meta_bvec2";} };
template<> struct clua_meta<math::BVec3> { static char const *name() {return "__meta_bvec3";} };
template<> struct clua_meta<math::BVec4> { static char const *name() {return "__meta_bvec4";} };
template<> struct clua_meta<math::Vec2i> { static char const *name() {return "__meta_ivec2";} };
template<> struct clua_meta<math::Vec3i> { static char const *name() {return "__meta_ivec3";} };
template<> struct clua_meta<math::Vec4i> { static char const *name() {return "__meta_ivec4";} };
template<> struct clua_meta<math::Vec2f> { static char const *name() {return "__meta_vec2";} };
template<> struct clua_meta<math::Vec3f> { static char const *name() {return "__meta_vec3";} };
template<> struct clua_meta<math::Vec4f> { static char const *name() {return "__meta_vec4";} };
template<> struct clua_meta<math::Quatf> { static char const *name() {return "__meta_quat";} };
template<> struct clua_meta<image::Image> { static char const *name() {return "__meta_image";} };

template<class T>
struct clua_name {};

template<> struct clua_name<math::BVec2> { static char const *name() {return "g_bvec2";} };
template<> struct clua_name<math::BVec3> { static char const *name() {return "g_bvec3";} };
template<> struct clua_name<math::BVec4> { static char const *name() {return "g_bvec4";} };
template<> struct clua_name<math::Vec2i> { static char const *name() {return "g_ivec2";} };
template<> struct clua_name<math::Vec3i> { static char const *name() {return "g_ivec3";} };
template<> struct clua_name<math::Vec4i> { static char const *name() {return "g_ivec4";} };
template<> struct clua_name<math::Vec2f> { static char const *name() {return "g_vec2";} };
template<> struct clua_name<math::Vec3f> { static char const *name() {return "g_vec3";} };
template<> struct clua_name<math::Vec4f> { static char const *name() {return "g_vec4";} };
template<> struct clua_name<math::Quatf> { static char const *name() {return "g_quat";} };

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

/**
 * @brief Write JSON method entries for all functions in a Lua table at the top of the stack.
 *
 * Iterates the table at the top of the Lua stack, and for each string-keyed function entry
 * (skipping names starting with '_'), writes a JSON object to the stream. If a jsonhelp
 * function is registered for the method, its output is used; otherwise a simple {"name":"..."}
 * is written.
 *
 * @param s The Lua state. The table to iterate must be at the top of the stack.
 * @param metaName The metatable name used to look up jsonhelp functions.
 * @param stream The output stream to write to.
 * @param[in,out] firstMethod Tracks whether a comma separator is needed. Set to true initially.
 * @return true on success, false on write error.
 */
bool clua_writejsonmethods(lua_State *s, const char *metaName, io::WriteStream &stream, bool &firstMethod);

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
inline int clua_push<math::Quatf>(lua_State* s, const math::Quatf& v) {
	lua_newtable(s);

	for (int i = 0; i < 4; ++i) {
		lua_pushstring(s, VEC_MEMBERS[i]);
		LuaNumberFuncs<math::Quatf::value_type>::push(s, v[i]);
		lua_settable(s, -3);
	}

	luaL_getmetatable(s, clua_meta<math::Quatf>::name());
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

math::Quatf clua_toquat(lua_State *s, int n);
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

template<class V>
int clua_pushvec(lua_State* s, const V& v) {
	lua_newtable(s);

	for (int i = 0; i < V::length(); ++i) {
		lua_pushstring(s, VEC_MEMBERS[i]);
		LuaNumberFuncs<typename V::value_type>::push(s, v[i]);
		lua_settable(s, -3);
	}

	luaL_getmetatable(s, clua_meta<V>::name());
	lua_setmetatable(s, -2);
	return 1;
}

template<> inline int clua_push<math::Vec2i>(lua_State* s, const math::Vec2i& v) { return clua_pushvec(s, v); }
template<> inline int clua_push<math::Vec3i>(lua_State* s, const math::Vec3i& v) { return clua_pushvec(s, v); }
template<> inline int clua_push<math::Vec4i>(lua_State* s, const math::Vec4i& v) { return clua_pushvec(s, v); }
template<> inline int clua_push<math::Vec2f>(lua_State* s, const math::Vec2f& v) { return clua_pushvec(s, v); }
template<> inline int clua_push<math::Vec3f>(lua_State* s, const math::Vec3f& v) { return clua_pushvec(s, v); }
template<> inline int clua_push<math::Vec4f>(lua_State* s, const math::Vec4f& v) { return clua_pushvec(s, v); }

inline float clua_optcomp(lua_State* s, int i, float value) {
	return (float)luaL_optnumber(s, i, value);
}

inline int clua_optcomp(lua_State* s, int i, int value) {
	return (int)luaL_optinteger(s, i, value);
}

template<class T>
int clua_vecadd(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a + b;
		return clua_pushvec(s, c);
	}
	const T& c = a + (typename T::value_type)lua_tonumber(s, 2);
	return clua_pushvec(s, c);
}

template<class T>
int clua_vecdiv(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a / b;
		return clua_pushvec(s, c);
	}
	const T& c = a / (typename T::value_type)lua_tonumber(s, 2);
	return clua_pushvec(s, c);
}

template<class T>
int clua_vecmul(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	if (clua_isvec<T>(s, 2)) {
		const T& b = clua_tovec<T>(s, 2);
		const T& c = a * b;
		return clua_pushvec(s, c);
	}
	const T& c = a * (typename T::value_type)lua_tonumber(s, 2);
	return clua_pushvec(s, c);
}

template<class T>
struct clua_vecequal {
static int equal(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	const T& b = clua_tovec<T>(s, 2);
	bool e = true;
	for (int i = 0; i < T::length(); ++i) {
		const double d = (double)a[i] - (double)b[i];
		if (d > 0.0001 || -d > 0.0001) {
			e = false;
		}
	}
	lua_pushboolean(s, e);
	return 1;
}
};

template<class T>
int clua_vecsub(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	const T& b = clua_tovec<T>(s, 2);
	const T& c = a - b;
	clua_pushvec(s, c);
	return 1;
}

template<class T>
int clua_vecnegate(lua_State* s) {
	const T& a = clua_tovec<T>(s, 1);
	clua_pushvec(s, -a);
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
struct clua_vecnew {
static int exec(lua_State* s) {
	T array(0);
	typename T::value_type value = 0;
	for (int i = 0; i < T::length(); ++i) {
		array[i] = clua_optcomp(s, i + 1, value);
		value = array[i];
	}
	return clua_pushvec(s, array);
}
};

template<class T>
struct clua_vecfunc {
	static int normalize(lua_State *s) {
		return clua_error(s, "normalize is not supported for this vector type");
	}
	static int length(lua_State *s) {
		return clua_error(s, "length is not supported for this vector type");
	}
	static int distance(lua_State *s) {
		return clua_error(s, "distance is not supported for this vector type");
	}
	static int dot(lua_State *s) {
		return clua_error(s, "dot is not supported for this vector type");
	}
};

#define CLUA_VECFUNC_FLOAT(V)                                                                                                  \
	template<>                                                                                                                 \
	struct clua_vecfunc<V> {                                                                                                   \
		static int normalize(lua_State *s) {                                                                                   \
			if (lua_isnumber(s, 1)) {                                                                                          \
				V v1(0);                                                                                                       \
				for (int i = 0; i < V::length(); ++i) {                                                                        \
					v1[i] = (float)lua_tonumber(s, i + 1);                                                                     \
				}                                                                                                              \
				return clua_pushvec(s, math::normalize(v1));                                                                   \
			}                                                                                                                  \
			return clua_pushvec(s, math::normalize(clua_tovec<V>(s, 1)));                                                      \
		}                                                                                                                      \
		static int length(lua_State *s) {                                                                                      \
			if (lua_isnumber(s, 1)) {                                                                                          \
				V v1(0);                                                                                                       \
				for (int i = 0; i < V::length(); ++i) {                                                                        \
					v1[i] = (float)lua_tonumber(s, i + 1);                                                                     \
				}                                                                                                              \
				lua_pushnumber(s, (double)math::length(v1));                                                                   \
				return 1;                                                                                                      \
			}                                                                                                                  \
			lua_pushnumber(s, (double)math::length(clua_tovec<V>(s, 1)));                                                      \
			return 1;                                                                                                          \
		}                                                                                                                      \
		static int distance(lua_State *s) {                                                                                    \
			lua_pushnumber(s, (double)math::distance(clua_tovec<V>(s, 1), clua_tovec<V>(s, 2)));                               \
			return 1;                                                                                                          \
		}                                                                                                                      \
		static int dot(lua_State *s) {                                                                                         \
			lua_pushnumber(s, (double)math::dot(clua_tovec<V>(s, 1), clua_tovec<V>(s, 2)));                                     \
			return 1;                                                                                                          \
		}                                                                                                                      \
	};

CLUA_VECFUNC_FLOAT(math::Vec2f)
CLUA_VECFUNC_FLOAT(math::Vec3f)
CLUA_VECFUNC_FLOAT(math::Vec4f)
#undef CLUA_VECFUNC_FLOAT

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
