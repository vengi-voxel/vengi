/**
 * @file
 */

#pragma once

#include "Container.h"
#include "core/ReadWriteLock.h"

namespace attrib {

/**
 * @brief Attributes are applied via @c Container instances
 *
 * @sa ContainerProvider
 */
class Attributes {
protected:
	std::atomic_bool _dirty;
	Values _current;
	Values _max;
	Containers _containers;
	// keep them here for ref counting
	std::unordered_set<ContainerPtr> _containerPtrs;
	core::ReadWriteLock _lock;
	core::ReadWriteLock _attribLock;

public:
	Attributes();

	/**
	 * @brief Calculates the new max values for the currently assigned @c Container's
	 */
	bool onFrame(long dt);

	void add(const Container& container);
	void add(Container&& container);
	void add(const ContainerPtr& container);
	void remove(const Container& container);
	void remove(Container&& container);
	void remove(const ContainerPtr& container);

	/**
	 * @brief Set the current value for a particular type. The current value is always capped
	 * by the max value (if there is one set) for that particular type.
	 *
	 * @param[in] type The attribute type
	 * @param[in] value The value to assign to the specified type
	 */
	double setCurrent(Types type, double value);
	/**
	 * @return The capped current value for the specified type
	 */
	double getCurrent(Types type) const;
	/**
	 * @return The current calculated max value for the specified type. This value is computed by the
	 * @c Container's that were added before the last @c update() call happened.
	 */
	double getMax(Types type) const;
};

inline double Attributes::getCurrent(Types type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	auto i = _current.find(type);
	if (i == _current.end())
		return 0.0;
	return i->second;
}

inline double Attributes::getMax(Types type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	auto i = _max.find(type);
	if (i == _max.end())
		return 0.0;
	return i->second;
}

}
