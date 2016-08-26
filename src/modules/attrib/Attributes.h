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
 * Containers are providing attribute types with values assigned, we have absolute and relative values.
 *
 * The attributes system will calculate a final value by building the sum over all absolute values, and
 * multiplies them by the sum of all relative values for one particular attribute type. That means that
 * if you e.g. apply a container that offers a damage type with a value of 10, another one with damage
 * type and a value of 10, too (both absolute) and last but not least a type damage with 10% relative, you
 * would get 22 as a final result.
 *
 * The system takes care about updating values in the @c Attributes::onFrame() method. Adding and removing
 * @c Container instances will set the dirty flag and will lead to a recalculation of the final values.
 *
 * The max values that are calculated here are just one value that this system provides. There are also the
 * current values provided. Let's take hit points as an example. You will have your current hit points, and
 * your max allowed hit points. The current hit points must be maintained by your game logic. E.g. you take
 * damage, so make sure to update your current hit points.
 *
 * The system is thread safe. There are two looks in the system - one that is locked if you modify attributes,
 * and one for adding and removing containers. The added/removed containers only lead to a re-evaluation of
 * the max values if @c Attributes::onFrame() was called.
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
	Attributes* _parent;

	void calculateMax(Values& max) const;

public:
	/**
	 * @param parent @c Attributes instance that can also contribute to your max values, but that
	 * are maintained in a different @c Attributes instance. This is useful to model a behaviour
	 * that for example each weapon has its own range and damage value, but still can be buffed
	 * by a global player state.
	 *
	 * @note The parent is optional, it is not modified, except that if it is dirty an update will be
	 * performed in @c onFrame()
	 */
	Attributes(Attributes* parent = nullptr);

	/**
	 * @brief Calculates the new max values for the currently assigned @c Container's
	 */
	bool onFrame(long dt);

	/**
	 * @note Locks the object (container)
	 */
	void add(const Container& container);
	/**
	 * @note Locks the object (container)
	 */
	void add(Container&& container);
	/**
	 * @note Locks the object (container)
	 */
	void add(const ContainerPtr& container);
	/**
	 * @note Locks the object (container)
	 */
	void remove(const Container& container);
	/**
	 * @note Locks the object (container)
	 */
	void remove(Container&& container);
	/**
	 * @note Locks the object (container)
	 */
	void remove(const ContainerPtr& container);

	/**
	 * @brief Set the current value for a particular type. The current value is always capped
	 * by the max value (if there is one set) for that particular type.
	 *
	 * @note Locks the object (attrib)
	 *
	 * @param[in] type The attribute type
	 * @param[in] value The value to assign to the specified type
	 */
	double setCurrent(Types type, double value);
	/**
	 * @note Locks the object (attrib)
	 *
	 * @return The capped current value for the specified type
	 */
	double getCurrent(Types type) const;
	/**
	 * @note Locks the object (attrib)
	 *
	 * @return The current calculated max value for the specified type. This value is computed by the
	 * @c Container's that were added before the last @c update() call happened.
	 */
	double getMax(Types type) const;
};

inline double Attributes::getCurrent(Types type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	auto i = _current.find(type);
	if (i == _current.end()) {
		return 0.0;
	}
	return i->second;
}

inline double Attributes::getMax(Types type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	auto i = _max.find(type);
	if (i == _max.end()) {
		return 0.0;
	}
	return i->second;
}

}
