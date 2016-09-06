/**
 * @file
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <memory>
#include "AttributeType.h"

namespace attrib {

class Container;

typedef std::unordered_set<Container> Containers;
typedef std::unordered_map<Type, double> Values;
typedef Values::const_iterator ValuesConstIter;
typedef Values::iterator ValuesIter;

/**
 * @brief A container provides percentage and absolute values for the @c Attributes instances. Containers
 * are applied to it and modify the final value for a @c Types with its provided absolute and percentage values.
 *
 * @sa Attributes
 */
class Container {
protected:
	std::string _name;
	Values _percentage;
	Values _absolute;
	int _stackCount;
	int _stackLimit;

public:
	Container(const std::string& name, const Values& percentage, const Values& absolute, int stackCount = 1, int stackLimit = 1) :
			_name(name), _percentage(percentage), _absolute(absolute), _stackCount(stackCount), _stackLimit(stackLimit) {
	}

	Container(std::string&& name, Values&& percentage, Values&& absolute, int stackCount = 1, int stackLimit = 1) :
			_name(std::move(name)), _percentage(std::move(percentage)), _absolute(std::move(absolute)), _stackCount(stackCount), _stackLimit(stackLimit) {
	}

	/**
	 * @brief Each container must have a unique name set.
	 */
	inline const std::string& name() const {
		return _name;
	}

	/**
	 * @return The percentage values that this container provides
	 * @sa absolute()
	 */
	inline const Values& percentage() const {
		return _percentage;
	}

	/**
	 * @return The absolute values that this container provides
	 * @sa percentage()
	 */
	inline const Values& absolute() const {
		return _absolute;
	}

	inline bool operator==(const Container& rhs) const {
		return rhs._name == _name;
	}
};

class ContainerBuilder {
private:
	Values _percentage;
	Values _absolute;
	const std::string _name;
	int _stackLimit;
public:
	ContainerBuilder(const std::string& name, int stackLimit = 1);
	ContainerBuilder& addPercentage(Type type, double value);
	ContainerBuilder& addAbsolute(Type type, double value);

	Container create() const;
};

typedef std::shared_ptr<Container> ContainerPtr;

inline Container ContainerBuilder::create() const {
	return Container(_name, _percentage, _absolute, 1, _stackLimit);
}

}

namespace std {
template<> struct hash<attrib::Container> {
	inline size_t operator()(const attrib::Container &c) const {
		return std::hash<std::string>()(c.name());
	}
};
}
