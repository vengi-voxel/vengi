/**
 * @file
 */

#include "LUAAttrib.h"
#include "attrib/Container.h"
#include "attrib/ContainerProvider.h"
#include "commonlua/LUAFunctions.h"

namespace attrib {

static const char* luaanim_containerproviderid() {
	return "__global_provider";
}

static const char* luaanim_metacontainer() {
	return "__meta_container";
}

static const char* luaanim_metaattrib() {
	return "__meta_attrib";
}

static Container* luaattrib_tocontainer(lua_State* s, int n) {
	return *(Container**)clua_getudata<Container*>(s, n, luaanim_metacontainer());
}

static int luaattrib_container_gc(lua_State * l) {
	return 0;
}

static int luaattrib_container_tostring(lua_State * l) {
	const Container *ctx = luaattrib_tocontainer(l, 1);
	if (ctx == nullptr) {
		return clua_error(l, "Expected to get container as first argument for __tostring");
	}
	lua_pushfstring(l, "container[name: %s]", ctx->name().c_str());
	return 1;
}

static int luaattrib_container_addabsolute(lua_State * l) {
	Container *ctx = luaattrib_tocontainer(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	if (ctx == nullptr) {
		return clua_error(l, "Expected to get container as first argument for addAbsolute(%s, %f)", type, value);
	}
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		return clua_error(l, "Unknown type given for addAbsolute(%s, %f)", type, value);
	}
	Values v = ctx->absolute();
	v[core::enumVal(attribType)] = value;
	ctx->setAbsolute(v);
	return 0;
}

static int luaattrib_container_setstacklimit(lua_State * l) {
	Container *ctx = luaattrib_tocontainer(l, 1);
	const int limit = luaL_checkinteger(l, 2);
	if (ctx == nullptr) {
		return clua_error(l, "Expected to get container as first argument for setStackLimit(%i)", limit);
	}
	ctx->setStackLimit(limit);
	return 0;
}

static int luaattrib_container_addpercentage(lua_State * l) {
	Container *ctx = luaattrib_tocontainer(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	if (ctx == nullptr) {
		return clua_error(l, "Expected to get container as first argument for addPercentage(%s, %f)", type, value);
	}
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		return clua_error(l, "Unknown type given for addPercentage(%s, %f)", type, value);
	}
	Values v = ctx->percentage();
	v[core::enumVal(attribType)] = value;
	ctx->setPercentage(v);
	return 0;
}

static int luaattrib_container_getname(lua_State * l) {
	const Container *ctx = luaattrib_tocontainer(l, 1);
	if (ctx == nullptr) {
		return clua_error(l, "Expected to get container as first argument for getName()");
	}
	lua_pushstring(l, ctx->name().c_str());
	return 1;
}

static ContainerProvider* luaattrib_getcontainerprovider(lua_State *s) {
	lua_getglobal(s, luaanim_containerproviderid());
	ContainerProvider *provider = (ContainerProvider *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return provider;
}

static void luaattrib_pushcontainerprovider(lua_State* s, ContainerProvider* provider) {
	lua_pushlightuserdata(s, provider);
	lua_setglobal(s, luaanim_containerproviderid());
}

static int luaattrib_pushcontainer(lua_State* s, Container *container) {
	return clua_pushudata(s, container, luaanim_metacontainer());
}

static int luaattrib_provider_create_container(lua_State * l) {
	ContainerProvider *ctx = luaattrib_getcontainerprovider(l);
	const char *name = luaL_checkstring(l, 1);
	if (ctx == nullptr) {
		return clua_error(l, "Unable to find Provider for createContainer(%s)", name);
	}
	const ContainerPtr& container = ctx->createContainer(name);
	if (!container) {
		lua_pushnil(l);
		return 1;
	}
	return luaattrib_pushcontainer(l, container.get());
}

static int luaattrib_provider_tostring(lua_State* l) {
	ContainerProvider *ctx = luaattrib_getcontainerprovider(l);
	if (ctx == nullptr) {
		return clua_error(l, "Unable to find Provider for createContainer");
	}
	const size_t size = ctx->containers().size();
	lua_pushfstring(l, "containers[amount: %d]", (int)size);
	return 1;
}

void luaattrib_setup(lua_State* s, ContainerProvider* provider) {
	static const luaL_Reg containerFuncs[] = {
		{"name",          luaattrib_container_getname},
		{"addAbsolute",   luaattrib_container_addabsolute},
		{"addPercentage", luaattrib_container_addpercentage},
		{"setStackLimit", luaattrib_container_setstacklimit},
		{"__gc",          luaattrib_container_gc},
		{"__tostring",    luaattrib_container_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, containerFuncs, luaanim_metacontainer());

	static const luaL_Reg attribFuncs[] = {
		{"createContainer", luaattrib_provider_create_container},
		{"__tostring",      luaattrib_provider_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, attribFuncs, luaanim_metaattrib(), "attrib");

	luaattrib_pushcontainerprovider(s, provider);

	clua_mathregister(s);
}

}
