/**
 * @file
 */

#include "ContainerProvider.h"
#include "commonlua/LUA.h"
#include "LUAFunctions.h"
#include "core/Log.h"

namespace attrib {

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
	container.addFunction("absolute", luaContainerAddAbsolute);
	container.addFunction("percentage", luaContainerAddPercentage);
	container.addFunction("register", luaContainerRegister);
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

void ContainerProvider::addContainer(const ContainerPtr& container) {
	if (!container) {
		return;
	}
	Log::trace("register container %s", container->name().c_str());
	if (_containers.find(container->name()) != _containers.end()) {
		Log::warn("overriding already existing container for %s", container->name().c_str());
	}
	_containers[container->name()] = container;
}

ContainerPtr ContainerProvider::container(const core::String& name) const {
	auto i = _containers.find(name);
	if (i == _containers.end()) {
		return ContainerPtr();
	}

	return i->second;
}

}
