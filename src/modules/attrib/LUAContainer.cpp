/**
 * @file
 */

#include "LUAContainer.h"
#include "ContainerProvider.h"
#include <memory>

namespace attrib {

LUAContainer::LUAContainer(const core::String& name, ContainerProvider* ctx) :
		_name(name), _ctx(ctx) {
}

void LUAContainer::addPercentage(Type type, double value) {
	_percentage.put(type, value);
}

void LUAContainer::addAbsolute(Type type, double value) {
	_absolute.put(type, value);
}

bool LUAContainer::registered() const {
	return _name.empty();
}

void LUAContainer::createContainer() {
	_ctx->addContainer(std::make_shared<Container>(_name, _percentage, _absolute));
	_percentage.clear();
	_absolute.clear();
	_name.clear();
}

const core::String& LUAContainer::name() const {
	return _name;
}

}
