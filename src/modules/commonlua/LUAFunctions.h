/**
 * @file
 */
#pragma once

#include "LUA.h"
#include "core/Log.h"
#include "core/GLM.h"
#include <vector>

template<class T>
struct clua_meta {};

template<> struct clua_meta<glm::ivec2> { static char const *name() {return "__meta_ivec2";} };
template<> struct clua_meta<glm::ivec3> { static char const *name() {return "__meta_ivec3";} };
template<> struct clua_meta<glm::ivec4> { static char const *name() {return "__meta_ivec4";} };
template<> struct clua_meta<glm::vec2> { static char const *name() {return "__meta_vec2";} };
template<> struct clua_meta<glm::vec3> { static char const *name() {return "__meta_vec3";} };
template<> struct clua_meta<glm::vec4> { static char const *name() {return "__meta_vec4";} };

template<class T>
struct clua_name {};

template<> struct clua_name<glm::ivec2> { static char const *name() {return "ivec2";} };
template<> struct clua_name<glm::ivec3> { static char const *name() {return "ivec3";} };
template<> struct clua_name<glm::ivec4> { static char const *name() {return "ivec4";} };
template<> struct clua_name<glm::vec2> { static char const *name() {return "vec2";} };
template<> struct clua_name<glm::vec3> { static char const *name() {return "vec3";} };
template<> struct clua_name<glm::vec4> { static char const *name() {return "vec4";} };

extern int clua_assignmetatable(lua_State* s, const char *name);

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
		std::string error(name);
		error.append(" userdata must not be null");
		luaL_argcheck(s, dataVoid != nullptr, n, error.c_str());
	}
	return (T) dataVoid;
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
struct clua_vecequal<glm::vec<N, int> > {
static int equal(lua_State* s) {
	const glm::vec<N, int>* a = clua_get<glm::vec<N, int> >(s, 1);
	const glm::vec<N, int>* b = clua_get<glm::vec<N, int> >(s, 2);
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
	for (size_t i = 0; i < sizeof(array) / sizeof(array[0]); ++i) {
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
	for (size_t i = 0; i < sizeof(array) / sizeof(array[0]); ++i) {
		array[i] = luaL_optinteger(s, i + 1, value);
		value = array[i];
	}
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
struct LuaNumberFuncs<int> {
	static void push(lua_State *s, int n) {
		lua_pushinteger(s, n);
	}

	static int check(lua_State *s, int arg) {
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
	const std::vector<luaL_Reg> funcs = {
		{"__add", clua_vecadd<T>},
		{"__sub", clua_vecsub<T>},
		{"__mul", clua_vecdot<T>::dot},
		{"__div", clua_vecdiv<T>},
		{"__unm", clua_vecnegate<T>},
		{"__len", clua_veclen<T>::len},
		{"__eq", clua_vecequal<T>::equal},
		{"__tostring", clua_vectostring<T>},
		{"__index", clua_vecindex<T>},
		{"__newindex", clua_vecnewindex<T>},
		{"dot", clua_vecdot<T>::dot},
		{nullptr, nullptr}
	};
	using RAWTYPE = typename std::remove_pointer<T>::type;
	clua_registerfuncs(s, &funcs.front(), clua_meta<RAWTYPE>::name());

	const std::vector<luaL_Reg> globalFuncs = {
		{"new", clua_vecnew<T>::vecnew},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, &globalFuncs.front(), clua_meta<RAWTYPE>::name(), clua_name<RAWTYPE>::name());
}

extern bool clua_optboolean(lua_State* s, int index, bool defaultVal);

extern int clua_typerror (lua_State *L, int narg, const char *tname);

extern int clua_checkboolean(lua_State *s, int index);
