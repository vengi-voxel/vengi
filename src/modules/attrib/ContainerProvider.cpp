#include "ContainerProvider.h"
#include "commonlua/LUA.h"
#include "LUAFunctions.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"

namespace attrib {

ContainerProvider::ContainerProvider() {
}

bool ContainerProvider::init() {
	lua::LUA lua;
	luaL_Reg createContainer = { "createContainer", luaCreateContainer };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { createContainer, eof };

	lua::LUAType container = lua.registerType("Container");
	container.addFunction("getName", luaContainerGetName);
	container.addFunction("absolute", luaContainerAddAbsolute);
	container.addFunction("percentage", luaContainerAddPercentage);
	container.addFunction("register", luaContainerRegister);
	container.addFunction("__gc", luaContainerGC);
	container.addFunction("__tostring", luaContainerToString);

	lua.reg("attrib", funcs);

	const std::string& attributes = core::App::getInstance()->filesystem()->load("attributes.lua");
	if (attributes.empty())
		return false;

	if (!lua.load(attributes)) {
		_error = lua.getError();
		return false;
	}

	// loads all the attributes
	lua.newGlobalData<ContainerProvider>("Provider", this);
	if (!lua.execute("init")) {
		_error = lua.getError();
		return false;
	}

	Log::info("loaded %i containers", (int)_containers.size());

	return true;
}

void ContainerProvider::addContainer(const ContainerPtr& container) {
	if (!container)
		return;
	Log::trace("register container %s", container->name().c_str());
	if (_containers.find(container->name()) != _containers.end())
		Log::warn("overriding already existing container for %s", container->name().c_str());
	_containers[container->name()] = container;
}

ContainerPtr ContainerProvider::getContainer(const std::string& name) const {
	auto i = _containers.find(name);
	if (i == _containers.end())
		return ContainerPtr();

	return i->second;
}

}
