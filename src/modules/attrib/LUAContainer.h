/**
 * @file
 */

#pragma once

#include "ContainerValues.h"
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
	LUAContainer(const core::String& name, ContainerProvider* ctx);

	void addPercentage(Type type, double value);

	void addAbsolute(Type type, double value);

	bool registered() const;

	void createContainer();

	const core::String& name() const;
};

}
