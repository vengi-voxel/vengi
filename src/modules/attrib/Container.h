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
public:
	Container(const std::string& name, const Values& percentage, const Values& absolute) :
			_name(name), _percentage(percentage), _absolute(absolute) {
	}

	Container(std::string&& name, Values&& percentage, Values&& absolute) :
			_name(std::move(name)), _percentage(std::move(percentage)), _absolute(std::move(absolute)) {
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
public:
	ContainerBuilder(const std::string& name);
	ContainerBuilder& addPercentage(Type type, double value);
	ContainerBuilder& addAbsolute(Type type, double value);

	Container create();
};

typedef std::shared_ptr<Container> ContainerPtr;

}

namespace std {
template<> struct hash<attrib::Container> {
	inline size_t operator()(const attrib::Container &c) const {
		return std::hash<std::string>()(c.name());
	}
};
}
