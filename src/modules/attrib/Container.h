/**
 * @file
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <memory>
#include "Types.h"

namespace attrib {

class Container;

typedef std::unordered_set<Container> Containers;
typedef std::unordered_map<Types, double> Values;
typedef Values::const_iterator ValuesConstIter;
typedef Values::iterator ValuesIter;

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

	inline const Values& percentage() const {
		return _percentage;
	}

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
	ContainerBuilder& addPercentage(Types type, double value);
	ContainerBuilder& addAbsolute(Types type, double value);

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
