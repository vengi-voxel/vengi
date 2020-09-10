/**
 * @file
 */

#include "ContainerProvider.h"
#include "attrib/Container.h"
#include "commonlua/LUA.h"
#include "attrib/Container.h"
#include "attrib/LUAAttrib.h"
#include "commonlua/LUA.h"
#include "ContainerProvider.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/SharedPtr.h"

namespace attrib {

bool ContainerProvider::init(const core::String& luaScript) {
	if (luaScript.empty()) {
		_error = "empty lua script given";
		return false;
	}
	_error = "";

	lua::LUA lua;
	luaattrib_setup(lua, this);
	if (!lua.load(luaScript)) {
		_error = lua.error();
		return false;
	}
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
		Log::debug("Container %s already exists", name.c_str());
		return ContainerPtr();
	}
	Log::debug("Create container: %s", name.c_str());
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
