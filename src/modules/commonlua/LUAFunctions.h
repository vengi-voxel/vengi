#pragma once

#include "LUA.h"
#include "core/Log.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/epsilon.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <vector>

template<class T>
struct clua_meta {};

template<> struct clua_meta<glm::ivec2> { static char const *name() {return "__meta_ivec2";} };
template<> struct clua_meta<glm::ivec3> { static char const *name() {return "__meta_ivec3";} };
template<> struct clua_meta<glm::ivec4> { static char const *name() {return "__meta_ivec4";} };
template<> struct clua_meta<glm::vec2> { static char const *name() {return "__meta_vec2";} };
template<> struct clua_meta<glm::vec3> { static char const *name() {return "__meta_vec3";} };
template<> struct clua_meta<glm::vec4> { static char const *name() {return "__meta_vec4";} };

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
	return clua_pushudata<T>(s, v, clua_meta<T>::name());
}

template<class T>
T* clua_get(lua_State *s, int n) {
	return clua_getudata<T*>(s, n, clua_meta<T>::name());
}

extern void clua_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name);

template<class T>
int clua_vecadd(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const T& c = *a + *b;
	return clua_push(s, c);
}

template<class T>
int clua_vecdot(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const float c = glm::dot(*a, *b);
	lua_pushnumber(s, c);
	return 1;
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
int clua_veclen(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const float c = glm::length(*a);
	lua_pushnumber(s, c);
	return 1;
}

template<class T>
int clua_veceq(lua_State* s) {
	const T* a = clua_get<T>(s, 1);
	const T* b = clua_get<T>(s, 2);
	const bool e = glm::all(glm::epsilonEqual(*a, *b, 0.0001f));
	lua_pushboolean(s, e);
	return 1;
}

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
struct clua_vecindex {};

template<>
struct clua_vecindex<glm::vec3> {
	static int index(lua_State *s) {
		const glm::vec3* v = clua_get<glm::vec3>(s, 1);
		const char* i = luaL_checkstring(s, 2);

		switch (*i) {
		case '0':
		case 'x':
		case 'r':
			lua_pushnumber(s, v->x);
			break;
		case '1':
		case 'y':
		case 'g':
			lua_pushnumber(s, v->y);
			break;
		case '2':
		case 'z':
		case 'b':
			lua_pushnumber(s, v->z);
			break;
		default:
			lua_pushnil(s);
			break;
		}

		return 1;
	}
};


template<>
struct clua_vecindex<glm::vec4> {
	static int index(lua_State *s) {
		const glm::vec4* v = clua_get<glm::vec4>(s, 1);
		const char* i = luaL_checkstring(s, 2);

		switch (*i) {
		case '0':
		case 'x':
		case 'r':
			lua_pushnumber(s, v->x);
			break;
		case '1':
		case 'y':
		case 'g':
			lua_pushnumber(s, v->y);
			break;
		case '2':
		case 'z':
		case 'b':
			lua_pushnumber(s, v->z);
			break;
		case '3':
		case 'w':
		case 'a':
			lua_pushnumber(s, v->w);
			break;
		default:
			lua_pushnil(s);
			break;
		}

		return 1;
	}
};


template<class T>
struct clua_vecnewindex {};

template<>
struct clua_vecnewindex<glm::vec4> {
	static int newindex(lua_State *s) {
		glm::vec4* v = clua_get<glm::vec4>(s, 1);
		const char *i = luaL_checkstring(s, 2);
		const float t = luaL_checknumber(s, 3);

		switch (*i) {
		case '0':
		case 'x':
		case 'r':
			v->x = t;
			break;
		case '1':
		case 'y':
		case 'g':
			v->y = t;
			break;
		case '2':
		case 'z':
		case 'b':
			v->z = t;
			break;
		case '3':
		case 'w':
		case 'a':
			v->w = t;
			break;
		default:
			break;
		}

		return 1;
	}
};

template<class T>
void clua_vecregister(lua_State* s) {
	std::vector<luaL_Reg> funcs = {
		{"__add", clua_vecadd<T>},
		{"__sub", clua_vecsub<T>},
		{"__mul", clua_vecdot<T>},
		{"__div", clua_vecdiv<T>},
		{"__unm", clua_vecnegate<T>},
		{"__len", clua_veclen<T>},
		{"__eq", clua_veceq<T>},
		{"__tostring", clua_vectostring<T>},
		{"__index", clua_vecindex<T>::index},
		{"__newindex", clua_vecnewindex<T>::newindex},
		{"dot", clua_vecdot<T>},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, &funcs.front(), clua_meta<T>::name());
}

extern bool clua_optboolean(lua_State* s, int index, bool defaultVal);
