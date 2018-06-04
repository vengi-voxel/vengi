/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"
#include "ContainerProvider.h"
#include "LUAContainer.h"
#include "core/String.h"

namespace attrib {

static ContainerProvider* luaGetContext(lua_State * l) {
	return lua::LUA::globalData<ContainerProvider>(l, "Provider");
}

static LUAContainer* luaGetContainerContext(lua_State * l, int n) {
	return lua::LUA::userData<LUAContainer>(l, n, "Container");
}

static int luaCreateContainer(lua_State * l) {
	ContainerProvider *ctx = luaGetContext(l);
	const std::string name = luaL_checkstring(l, 1);
	lua::LUA::newUserdata(l, "Container", new LUAContainer(name, ctx));
	return 1;
}

static int luaContainerGC(lua_State * l) {
	LUAContainer *container = luaGetContainerContext(l, 1);
	if (!container->registered()) {
		const int ret = luaL_error(l, "Container '%s' wasn't registered", container->name().c_str());
		delete container;
		return ret;
	}
	delete container;
	return 0;
}

static int luaContainerToString(lua_State * l) {
	const LUAContainer *ctx = luaGetContainerContext(l, 1);
	lua_pushfstring(l, "container: %s", ctx->name().c_str());
	return 1;
}

static int luaContainerAddAbsolute(lua_State * l) {
	LUAContainer *ctx = luaGetContainerContext(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		const std::string& error = core::string::format("Unknown type given: %s", type);
		lua::LUA::returnError(l, error);
	}
	ctx->addAbsolute(attribType, value);
	return 0;
}

static int luaContainerAddPercentage(lua_State * l) {
	LUAContainer *ctx = luaGetContainerContext(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		const std::string& error = core::string::format("Unknown type given: %s", type);
		lua::LUA::returnError(l, error);
	}
	ctx->addPercentage(attribType, value);
	return 0;
}

static int luaContainerRegister(lua_State * l) {
	LUAContainer *ctx = luaGetContainerContext(l, 1);
	ctx->createContainer();
	return 0;
}

static int luaContainerGetName(lua_State * l) {
	const LUAContainer *ctx = luaGetContainerContext(l, 1);
	lua_pushstring(l, ctx->name().c_str());
	return 1;
}

}
