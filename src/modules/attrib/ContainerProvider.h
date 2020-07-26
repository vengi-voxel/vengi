/**
 * @file
 */

#pragma once

#include "Container.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/SharedPtr.h"

namespace attrib {

/**
 * @brief LUA container provider.
 *
 * LUA file example:
 * @code
 * function init()
 *  local example = attrib.createContainer("example")
 *  example:addAbsolute("ATTACKRANGE", 2.0)
 * end
 * @endcode
 * @ingroup Attributes
 */
class ContainerProvider {
public:
	typedef core::StringMap<ContainerPtr> Containers;
private:
	Containers _containers;
	core::String _error;
public:
	/**
	 * @param luaScript The lua script string to load
	 * @return @c true on success, @c false if an error occurred. In case of an error,
	 * you can call error() to get more information about it.
	 * @note this can be called multiple times. But beware, if a @c Container with the same
	 * name already exists, it will just be overwritten-
	 */
	bool init(const core::String& luaScript);

	/**
	 * @brief Removes all known containers from previous init() calls
	 */
	void reset();

	/**
	 * @return Immutable list of Container instances that were already parsed.
	 */
	const Containers& containers() const;

	void addContainer(const ContainerPtr& container);
	ContainerPtr container(const core::String& name) const;

	/**
	 * @note If a container with the given name already exists, this method returns a null pointer
	 */
	ContainerPtr createContainer(const core::String& name);

	/**
	 * @return The last error that occurred in an init() call
	 */
	inline const core::String& error() const {
		return _error;
	}
};

typedef core::SharedPtr<ContainerProvider> ContainerProviderPtr;

inline void ContainerProvider::reset() {
	_error = "";
	_containers.clear();
}

inline const ContainerProvider::Containers& ContainerProvider::containers() const {
	return _containers;
}

}
