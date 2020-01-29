/**
 * @file
 */

#pragma once

#include "Container.h"
#include <memory>
#include <unordered_map>
#include "core/String.h"

namespace attrib {

/**
 * @brief LUA container provider.
 *
 * LUA file example:
 * @code
 * function init()
 *  local example = attrib.createContainer("example")
 *  example:absolute("ATTACKRANGE", 2.0)
 *  example:register()
 * end
 * @endcode
 * @ingroup Attributes
 */
class ContainerProvider {
public:
	typedef std::unordered_map<std::string, ContainerPtr> Containers;
private:
	Containers _containers;
	std::string _error;
public:
	/**
	 * @param luaScript The lua script string to load
	 * @return @c true on success, @c false if an error occurred. In case of an error,
	 * you can call error() to get more information about it.
	 * @note this can be called multiple times. But beware, if a @c Container with the same
	 * name already exists, it will just be overwritten-
	 */
	bool init(const std::string& luaScript);

	/**
	 * @brief Removes all known containers from previous init() calls
	 */
	void reset();

	/**
	 * @return Immutable list of Container instances that were already parsed.
	 */
	const Containers& containers() const;

	void addContainer(const ContainerPtr& container);
	ContainerPtr container(const std::string& name) const;

	/**
	 * @return The last error that occurred in an init() call
	 */
	inline const std::string& error() const {
		return _error;
	}
};

typedef std::shared_ptr<ContainerProvider> ContainerProviderPtr;

inline void ContainerProvider::reset() {
	_error = "";
	_containers.clear();
}

inline const ContainerProvider::Containers& ContainerProvider::containers() const {
	return _containers;
}

}
