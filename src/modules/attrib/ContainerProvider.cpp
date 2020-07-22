/**
 * @file
 */

#include "ContainerProvider.h"
#include "attrib/Container.h"
#include "commonlua/LUA.h"
#include "attrib/Container.h"
#include "commonlua/LUA.h"
#include "ContainerProvider.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/SharedPtr.h"

namespace attrib {

static ContainerProvider* luaGetContext(lua_State * l) {
	return lua::LUA::globalData<ContainerProvider>(l, "Provider");
}

static Container* luaGetContainerContext(lua_State * l, int n) {
	return lua::LUA::userData<Container>(l, n, "Container");
}

static int luaCreateContainer(lua_State * l) {
	ContainerProvider *ctx = luaGetContext(l);
	const char *name = luaL_checkstring(l, 1);
	const ContainerPtr& container = ctx->createContainer(name);
	lua::LUA::newUserdata(l, "Container", container.get());
	return 1;
}

static int luaContainerGC(lua_State * l) {
	return 0;
}

static int luaContainerToString(lua_State * l) {
	const Container *ctx = luaGetContainerContext(l, 1);
	lua_pushfstring(l, "container: %s", ctx->name().c_str());
	return 1;
}

static int luaContainerAddAbsolute(lua_State * l) {
	Container *ctx = luaGetContainerContext(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		const core::String& error = core::string::format("Unknown type given: %s", type);
		lua::LUA::returnError(l, error);
	}
	Values v = ctx->absolute();
	v.put(attribType, value);
	ctx->setAbsolute(v);
	return 0;
}

static int luaContainerSetStackLimit(lua_State * l) {
	Container *ctx = luaGetContainerContext(l, 1);
	const int limit = luaL_checkinteger(l, 2);
	ctx->setStackLimit(limit);
	return 0;
}

static int luaContainerAddPercentage(lua_State * l) {
	Container *ctx = luaGetContainerContext(l, 1);
	const char* type = luaL_checkstring(l, 2);
	const double value = luaL_checknumber(l, 3);
	attrib::Type attribType = getType(type);
	if (attribType == attrib::Type::NONE) {
		const core::String& error = core::string::format("Unknown type given: %s", type);
		lua::LUA::returnError(l, error);
	}
	Values v = ctx->percentage();
	v.put(attribType, value);
	ctx->setPercentage(v);
	return 0;
}

static int luaContainerGetName(lua_State * l) {
	const Container *ctx = luaGetContainerContext(l, 1);
	lua_pushstring(l, ctx->name().c_str());
	return 1;
}

bool ContainerProvider::init(const core::String& luaScript) {
	if (luaScript.empty()) {
		_error = "empty lua script given";
		return false;
	}
	_error = "";

	lua::LUA lua;
	luaL_Reg createContainer = { "createContainer", luaCreateContainer };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { createContainer, eof };

	lua::LUAType container = lua.registerType("Container");
	container.addFunction("name", luaContainerGetName);
	container.addFunction("addAbsolute", luaContainerAddAbsolute);
	container.addFunction("addPercentage", luaContainerAddPercentage);
	container.addFunction("setStackLimit", luaContainerSetStackLimit);
	container.addFunction("__gc", luaContainerGC);
	container.addFunction("__tostring", luaContainerToString);

	lua.reg("attrib", funcs);

	if (!lua.load(luaScript)) {
		_error = lua.error();
		return false;
	}

	// loads all the attributes
	lua.newGlobalData<ContainerProvider>("Provider", this);
	if (!lua.execute("init")) {
		_error = lua.error();
		return false;
	}

	Log::info("loaded %i containers", (int)_containers.size());

	return true;
}

ContainerPtr ContainerProvider::createContainer(const core::String& name) {
	ContainerPtr c = container(name);
	if (c) {
		return ContainerPtr();
	}
	c = core::make_shared<Container>(name);
	addContainer(c);
	return c;
}

void ContainerProvider::addContainer(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	Log::trace("register container %s", container->name().c_str());
	if (_containers.find(container->name()) != _containers.end()) {
		Log::warn("overriding already existing container for %s", container->name().c_str());
	}
	_containers.put(container->name(), container);
}

ContainerPtr ContainerProvider::container(const core::String& name) const {
	auto i = _containers.find(name);
	if (i == _containers.end()) {
		return ContainerPtr();
	}

	return i->value;
}

}
