/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/SharedPtr.h"
#include "core/collection/Map.h"
#include "core/collection/StringMap.h"
#include "AttributeType.h"
#include "ContainerValues.h"

namespace attrib {

class Container;
typedef core::StringMap<Container> Containers;

/**
 * @brief A container provides percentage and absolute values for the @c Attributes instances. Containers
 * are applied to it and modify the final value for a @c Types with its provided absolute and percentage values.
 *
 * @sa Attributes
 * @ingroup Attributes
 */
class Container {
protected:
	core::String _name;
	Values _percentage;
	Values _absolute;
	int _stackCount;
	int _stackLimit;
	size_t _hash;

public:
	Container(const core::String& name, const Values& percentage, const Values& absolute, int stackCount = 1, int stackLimit = 1);

	Container(core::String&& name, Values&& percentage, Values&& absolute, int stackCount = 1, int stackLimit = 1);

	Container(const core::String& name);

	/**
	 * @brief Each container must have a unique name set.
	 */
	const core::String& name() const;

	/**
	 * @return The percentage values that this container provides
	 * @sa absolute()
	 */
	const Values& percentage() const;

	/**
	 * @return The absolute values that this container provides
	 * @sa percentage()
	 */
	const Values& absolute() const;

	void setPercentage(const Values& values);
	void setAbsolute(const Values& values);

	int stackCount() const;
	int stackLimit() const;

	void setStackCount(int stackCount);
	void setStackLimit(int stackLimit);

	bool increaseStackCount();
	bool decreaseStackCount();

	bool operator==(const Container& rhs) const;
	bool operator==(const Container& other);
};

typedef core::SharedPtr<Container> ContainerPtr;

class ContainerBuilder {
private:
	Values _percentage;
	Values _absolute;
	const core::String _name;
	int _stackLimit;
public:
	ContainerBuilder(const core::String& name, int stackLimit = 1);
	ContainerBuilder& setPercentage(Type type, double value);
	ContainerBuilder& setAbsolute(Type type, double value);

	Container create() const;
};

inline Container ContainerBuilder::create() const {
	return Container(_name, _percentage, _absolute, 1, _stackLimit);
}

}
