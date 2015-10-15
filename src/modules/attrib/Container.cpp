#include "Container.h"

namespace attrib {

ContainerBuilder::ContainerBuilder(const std::string& name) :
		_name(name) {
}

ContainerBuilder& ContainerBuilder::addPercentage(Types type, double value) {
	_percentage[type] = value;
	return *this;
}

ContainerBuilder& ContainerBuilder::addAbsolute(Types type, double value) {
	_absolute[type] = value;
	return *this;
}

Container ContainerBuilder::create() {
	return Container(_name, _percentage, _absolute);
}

}
