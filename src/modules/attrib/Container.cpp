/**
 * @file
 */

#include "Container.h"

namespace attrib {

Container::Container(const core::String& name, const Values& percentage, const Values& absolute, int stackCount, int stackLimit) :
		_name(name), _percentage(percentage), _absolute(absolute),
		_stackCount(stackCount), _stackLimit(stackLimit), _hash(core::StringHash{}(_name)) {
}

Container::Container(core::String&& name, Values&& percentage, Values&& absolute, int stackCount, int stackLimit) :
		_name(std::move(name)), _percentage(percentage), _absolute(absolute),
		_stackCount(stackCount), _stackLimit(stackLimit), _hash(core::StringHash{}(_name)) {
}

Container::Container(const core::String& name) :
	_name(name), _stackCount(1), _stackLimit(1), _hash(core::StringHash{}(_name)) {
}

const core::String& Container::name() const {
	return _name;
}

const Values& Container::percentage() const {
	return _percentage;
}

const Values& Container::absolute() const {
	return _absolute;
}

bool Container::operator==(const Container& rhs) const {
	return rhs._name == _name;
}

int Container::stackCount() const {
	return _stackCount;
}

void Container::setStackCount(int stackCount) {
	_stackCount = stackCount;
}

void Container::setStackLimit(int stackLimit) {
	_stackLimit = stackLimit;
}

void Container::setPercentage(const Values& values) {
	_percentage = values;
}

void Container::setAbsolute(const Values& values) {
	_absolute = values;
}

bool Container::increaseStackCount() {
	if (_stackCount < _stackLimit) {
		++_stackCount;
		return true;
	}
	return false;
}

bool Container::decreaseStackCount() {
	if (_stackCount > 0) {
		--_stackCount;
		return true;
	}
	return false;
}

int Container::stackLimit() const {
	return _stackLimit;
}

bool Container::operator==(const Container& other) {
	if (_hash != other._hash) {
		return false;
	}
	return _name == other._name;
}

ContainerBuilder::ContainerBuilder(const core::String& name, int stackLimit) :
		_name(name), _stackLimit(stackLimit) {
}

ContainerBuilder& ContainerBuilder::addPercentage(Type type, double value) {
	_percentage.put(type, value);
	return *this;
}

ContainerBuilder& ContainerBuilder::addAbsolute(Type type, double value) {
	_absolute.put(type, value);
	return *this;
}

}
