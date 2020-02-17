/**
 * @file
 */

#pragma once

#include <memory>
#include "core/String.h"

namespace attrib {

class ContainerProvider;

class LUAContainer {
private:
	core::String _name;
	ContainerProvider* _ctx;
	Values _percentage;
	Values _absolute;
public:
	LUAContainer(const core::String& name, ContainerProvider* ctx) :
			_name(name), _ctx(ctx) {
	}

	void addPercentage(Type type, double value) {
		_percentage.put(type, value);
	}

	void addAbsolute(Type type, double value) {
		_absolute.put(type, value);
	}

	inline bool registered() const {
		return _name.empty();
	}

	void createContainer() {
		_ctx->addContainer(std::make_shared<Container>(_name, _percentage, _absolute));
		_percentage.clear();
		_absolute.clear();
		_name.clear();
	}

	inline const core::String& name() const {
		return _name;
	}
};

}
