/**
 * @file
 */

#pragma once

#include "Container.h"
#include "core/concurrent/Concurrency.h"
#include "core/concurrent/ReadWriteLock.h"
#include "core/concurrent/Atomic.h"
#include <functional>
#include <vector>

#undef max

namespace attrib {

struct DirtyValue {
	attrib::Type type = attrib::Type::NONE;
	bool current = false;
	double value = 0.0;

	inline bool operator==(const DirtyValue& rhs) const {
		return type == rhs.type && current == rhs.current;
	}
};

}

namespace std {
template<> struct hash<attrib::DirtyValue> {
	inline size_t operator()(const attrib::DirtyValue &c) const {
		return std::hash<int>()(static_cast<int>(c.type));
	}
};

}

namespace attrib {

/**
 * @defgroup Attributes
 * @{
 * @brief Attributes are applied via @c Container instances
 *
 * Containers are providing attribute types (@c attrib::Type) with values assigned, we have absolute and
 * relative values.
 *
 * The attributes system will calculate a final value by building the sum over all absolute values, and
 * multiplies them by the sum of all relative values for one particular attribute type. That means that
 * if you e.g. apply a container that offers a damage type with a value of 10, another one with damage
 * type and a value of 10, too (both absolute) and last but not least a type damage with 10% relative, you
 * would get 22 as a final result.
 *
 * The system takes care about updating values in the @c Attributes::update() method. Adding and removing
 * @c Container instances will set the dirty flag and will lead to a recalculation of the final values.
 *
 * The max values that are calculated here are just one value that this system provides. There are also the
 * current values provided. Let's take hit points as an example. You will have your current hit points, and
 * your max allowed hit points. The current hit points must be maintained by your game logic. E.g. you take
 * damage, so make sure to update your current hit points.
 *
 * The system is thread safe. There are two looks in the system - one that is locked if you modify attributes,
 * and one for adding and removing containers. The added/removed containers only lead to a re-evaluation of
 * the max values if @c Attributes::update() was called.
 *
 * @sa ContainerProvider
 * @sa ShadowAttributes
 */
class Attributes {
protected:
	core::AtomicBool _dirty { false };
	Values _current core_thread_guarded_by(_attribLock);
	Values _max core_thread_guarded_by(_attribLock);
	Containers _containers core_thread_guarded_by(_lock);
	// keep them here for ref counting
	core::StringMap<ContainerPtr> _containerPtrs;
	core::ReadWriteLock _lock;
	core::ReadWriteLock _attribLock;
	Attributes* _parent;
	core::String _name = "unnamed";
	std::vector<std::function<void(const DirtyValue&)> > _listeners;

	void calculateMax(Values& absolutes, Values& percentages) const;

public:
	/**
	 * @param parent @c Attributes instance that can also contribute to your max values, but that
	 * are maintained in a different @c Attributes instance. This is useful to model a behaviour
	 * that for example each weapon has its own range and damage value, but still can be buffed
	 * by a global player state.
	 *
	 * @note The parent is optional, it is not modified, except that if it is dirty an update will be
	 * performed in @c update(). Only the max values are taken into account here (absolute and percentage
	 * modifiers) - but not the currents.
	 */
	Attributes(Attributes* parent = nullptr);

	/**
	 * @brief One entity can have several instances of the attributes system, to distinguish them easier,
	 * you can specify names for them.
	 * @sa name()
	 */
	void setName(const core::String& name);
	/**
	 * @return The name of the attributes system instance.
	 * @sa setName()
	 */
	const core::String& name() const;

	void markAsDirty();

	/**
	 * @brief Adds a new listener that will get notified whenever a @c attrib::Type value has changed.
	 * @param f The functor, lambda or method object. It has to accept @c attrib::DirtyValue.
	 */
	template<class F>
	void addListener(F&& f) {
		_listeners.emplace_back(std::forward<F>(f));
	}

	void clearListeners() {
		_listeners.clear();
	}

	/**
	 * @brief Calculates the new max values for the currently assigned @c Container's
	 */
	bool update(long dt);

	/**
	 * @note Locks the object (container)
	 */
	bool add(const Container& container);
	/**
	 * @note Locks the object (container)
	 */
	bool add(const ContainerPtr& container);
	/**
	 * @note Locks the object (container)
	 */
	void remove(const Container& container);
	/**
	 * @note Locks the object (container)
	 */
	void remove(const core::String& name);
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
	double setCurrent(Type type, double value);
	/**
	 * @note Locks the object (attrib)
	 *
	 * @return The capped current value for the specified type
	 */
	double current(Type type) const;
	/**
	 * @note Locks the object (attrib)
	 *
	 * @return The current calculated max value for the specified type. This value is computed by the
	 * @c Container's that were added before the last @c update() call happened.
	 */
	double max(Type type) const;
};

inline double Attributes::current(Type type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	return _current[core::enumVal(type)];
}

inline double Attributes::max(Type type) const {
	core::ScopedReadLock scopedLock(_attribLock);
	return _max[core::enumVal(type)];
}

inline void Attributes::setName(const core::String& name) {
	_name = name;
}

inline const core::String& Attributes::name() const {
	return _name;
}

/**
 * @}
 */

}
