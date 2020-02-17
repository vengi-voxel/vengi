/**
 * @file
 */

#include "Container.h"

namespace attrib {

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
