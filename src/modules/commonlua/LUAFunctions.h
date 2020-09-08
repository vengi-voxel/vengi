/**
 * @file
 */
#pragma once

#include "commonlua/LUA.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "core/StringUtil.h"
#include <lua.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>

extern void clua_assert(lua_State* s, bool pass, const char *msg);
extern void clua_assert_argc(lua_State* s, bool pass);

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

template<class T>
struct clua_name {};

template<> struct clua_name<glm::bvec2> { static char const *name() {return "bvec2";} };
template<> struct clua_name<glm::bvec3> { static char const *name() {return "bvec3";} };
template<> struct clua_name<glm::bvec4> { static char const *name() {return "bvec4";} };
template<> struct clua_name<glm::dvec2> { static char const *name() {return "dvec2";} };
template<> struct clua_name<glm::dvec3> { static char const *name() {return "dvec3";} };
template<> struct clua_name<glm::dvec4> { static char const *name() {return "dvec4";} };
template<> struct clua_name<glm::ivec2> { static char const *name() {return "ivec2";} };
template<> struct clua_name<glm::ivec3> { static char const *name() {return "ivec3";} };
template<> struct clua_name<glm::ivec4> { static char const *name() {return "ivec4";} };
template<> struct clua_name<glm::vec2> { static char const *name() {return "vec2";} };
template<> struct clua_name<glm::vec3> { static char const *name() {return "vec3";} };
template<> struct clua_name<glm::vec4> { static char const *name() {return "vec4";} };
template<> struct clua_name<glm::quat> { static char const *name() {return "quat";} };

extern int clua_assignmetatable(lua_State* s, const char *name);

extern int clua_ioloader(lua_State *s);

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
	void* dataVoid = luaL_checkudata(s, n, name);
	if (dataVoid == nullptr) {
		core::String error(name);
		error.append(" userdata must not be null");
		luaL_argcheck(s, dataVoid != nullptr, n, error.c_str());
	}
	return (T) dataVoid;
}

template<class T>
bool clua_istype(lua_State *s, int n) {
	using RAWTYPE = typename std::remove_pointer<T>::type;
	return luaL_testudata(s, n, clua_meta<RAWTYPE>::name()) != nullptr;
}

template<class T>
int clua_push(lua_State* s, const T& v) {
	using RAWTYPE = typename std::remove_pointer<T>::type;
	return clua_pushudata<T>(s, v, clua_meta<RAWTYPE>::name());
}

template<class T>
T* clua_get(lua_State *s, int n) {
	using RAWTYPE = typename std::remove_pointer<T>::type;
	return clua_getudata<T*>(s, n, clua_meta<RAWTYPE>::name());
}

extern void clua_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name);
extern void clua_registerfuncsglobal(lua_State* s, const luaL_Reg* funcs, const char *meta, const char *name);

template<class T>
int clua_vecadd(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const T& c = *a + *b;
	return clua_push(s, c);
}

template<class T>
int clua_vecdiv(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const T& c = *a / *b;
	clua_push(s, c);
	return 1;
}

template<class T>
int clua_vecmul(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const T& c = *a * *b;
	clua_push(s, c);
	return 1;
}

template<class T>
struct clua_veclen {
static int len(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const float c = glm::length(*a);
	lua_pushnumber(s, c);
	return 1;
}
};

template<int N>
struct clua_veclen<glm::vec<N, int> > {
static int len(lua_State* s) {
	return luaL_error(s, "'length' accepts only floating-point inputs");
}
};

template<int N>
struct clua_veclen<glm::vec<N, bool> > {
static int len(lua_State* s) {
	return luaL_error(s, "'length' accepts only floating-point inputs");
}
};

template<class T>
struct clua_vecdot {
static int dot(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const float c = glm::dot(*a, *b);
	lua_pushnumber(s, c);
	return 1;
}
};

template<int N>
struct clua_vecdot<glm::vec<N, int> > {
static int dot(lua_State* s) {
	return luaL_error(s, "'dot' accepts only floating-point inputs");
}
};

template<class T>
struct clua_vecequal {};

template<int N>
struct clua_vecequal<glm::vec<N, float> > {
static int equal(lua_State* s) {
	const glm::vec<N, float> * a = clua_get<glm::vec<N, float> >(s, 1);
	const glm::vec<N, float> * b = clua_get<glm::vec<N, float> >(s, 2);
	const bool e = glm::all(glm::epsilonEqual(*a, *b, 0.0001f));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, double> > {
static int equal(lua_State* s) {
	const glm::vec<N, double> * a = clua_get<glm::vec<N, double> >(s, 1);
	const glm::vec<N, double> * b = clua_get<glm::vec<N, double> >(s, 2);
	const bool e = glm::all(glm::epsilonEqual(*a, *b, 0.0001));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, int> > {
static int equal(lua_State* s) {
	const glm::vec<N, int>* a = clua_get<glm::vec<N, int> >(s, 1);
	const glm::vec<N, int>* b = clua_get<glm::vec<N, int> >(s, 2);
	const bool e = glm::all(glm::equal(*a, *b));
	lua_pushboolean(s, e);
	return 1;
}
};

template<int N>
struct clua_vecequal<glm::vec<N, bool> > {
static int equal(lua_State* s) {
	const glm::vec<N, bool>* a = clua_get<glm::vec<N, bool> >(s, 1);
	const glm::vec<N, bool>* b = clua_get<glm::vec<N, bool> >(s, 2);
	const bool e = glm::all(glm::equal(*a, *b));
	lua_pushboolean(s, e);
	return 1;
}
};

template<class T>
int clua_vecsub(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const T& c = *a - *b;
	clua_push(s, c);
	return 1;
}

template<class T>
int clua_vecnegate(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	clua_push<T>(s, -(*a));
	return 1;
}

template<class T>
int clua_vectostring(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	lua_pushfstring(s, "%s", glm::to_string(*a).c_str());
	return 1;
}

template<class T>
struct clua_vecnew {};

template<int N>
struct clua_vecnew<glm::vec<N, float> > {
static int vecnew(lua_State* s) {
	glm::vec<N, float> array;
	float value = 0.0f;
	for (size_t i = 0; i < N; ++i) {
		array[i] = luaL_optnumber(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, double> > {
static int vecnew(lua_State* s) {
	glm::vec<N, double> array;
	float value = 0.0f;
	for (size_t i = 0; i < N; ++i) {
		array[i] = luaL_optnumber(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, int> > {
static int vecnew(lua_State* s) {
	glm::vec<N, int> array;
	int value = 0;
	for (size_t i = 0; i < N; ++i) {
		array[i] = luaL_optinteger(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<int N>
struct clua_vecnew<glm::vec<N, bool> > {
static int vecnew(lua_State* s) {
	glm::vec<N, bool> array;
	int value = 0;
	for (size_t i = 0; i < N; ++i) {
		array[i] = luaL_optinteger(s, i + 1, value);
		value = array[i];
	}
	return clua_push(s, array);
}
};

template<>
struct clua_vecnew<glm::quat> {
static int vecnew(lua_State* s) {
	glm::quat array = glm::quat_identity<float, glm::defaultp>();
	return clua_push(s, array);
}
};
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

	static int check(lua_State *s, int arg) {
		return luaL_checknumber(s, arg);
	}
};

template<>
struct LuaNumberFuncs<int> {
	static void push(lua_State *s, int n) {
		lua_pushinteger(s, n);
	}

	static int check(lua_State *s, int arg) {
		return luaL_checkinteger(s, arg);
	}
};

template<>
struct LuaNumberFuncs<bool> {
	static void push(lua_State *s, bool n) {
		lua_pushinteger(s, n);
	}

	static bool check(lua_State *s, int arg) {
		return luaL_checkinteger(s, arg);
	}
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
	return luaL_error(s, "Invalid component %c", *i);
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
		return 1;
	case '1':
	case 'y':
	case 'g':
		if (T::length() >= 2) {
			(*v)[1] = t;
			return 1;
		}
		break;
	case '2':
	case 'z':
	case 'b':
		if (T::length() >= 3) {
			(*v)[2] = t;
			return 1;
		}
		break;
	case '3':
	case 'w':
	case 'a':
		if (T::length() >= 4) {
			(*v)[3] = t;
			return 1;
		}
		break;
	default:
		break;
	}
	return luaL_error(s, "Invalid component %c", *i);
}

template<class T>
void clua_vecregister(lua_State* s) {
	using RAWTYPE = typename std::remove_pointer<T>::type;
	const luaL_Reg funcs[] = {
		{"__add", clua_vecadd<RAWTYPE>},
		{"__sub", clua_vecsub<RAWTYPE>},
		{"__mul", clua_vecmul<RAWTYPE>},
		{"__div", clua_vecdiv<RAWTYPE>},
		{"__unm", clua_vecnegate<RAWTYPE>},
		{"__len", clua_veclen<RAWTYPE>::len},
		{"__eq", clua_vecequal<RAWTYPE>::equal},
		{"__tostring", clua_vectostring<RAWTYPE>},
		{"__index", clua_vecindex<RAWTYPE>},
		{"__newindex", clua_vecnewindex<RAWTYPE>},
		{"dot", clua_vecdot<RAWTYPE>::dot},
		{nullptr, nullptr}
	};
	Log::debug("Register %s lua functions", clua_meta<RAWTYPE>::name());
	clua_registerfuncs(s, funcs, clua_meta<RAWTYPE>::name());
	const luaL_Reg globalFuncs[] = {
		{"new", clua_vecnew<RAWTYPE>::vecnew},
		{nullptr, nullptr}
	};
	const core::String& globalMeta = core::string::format("%s_global", clua_meta<RAWTYPE>::name());
	clua_registerfuncsglobal(s, globalFuncs, globalMeta.c_str(), clua_name<RAWTYPE>::name());
}

extern void clua_quatregister(lua_State* s);

extern bool clua_optboolean(lua_State* s, int index, bool defaultVal);

extern int clua_typerror(lua_State *s, int narg, const char *tname);

extern int clua_checkboolean(lua_State *s, int index);

/**
 * @brief Registers all shared lua modules/globals/functions
 */
extern void clua_register(lua_State *s);
extern void clua_mathregister(lua_State *s);

extern void clua_cmdregister(lua_State *s);
extern void clua_varregister(lua_State *s);
extern void clua_logregister(lua_State *s);
