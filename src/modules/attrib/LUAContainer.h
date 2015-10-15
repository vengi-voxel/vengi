#pragma once

#include <string>

namespace attrib {

class ContainerProvider;

class LUAContainer {
private:
	std::string _name;
	ContainerProvider* _ctx;
	Values _percentage;
	Values _absolute;
public:
	LUAContainer(const std::string& name, ContainerProvider* ctx) :
			_name(name), _ctx(ctx) {
	}

	void addPercentage(Types type, double value) {
		_percentage.insert(std::make_pair(type, value));
	}

	void addAbsolute(Types type, double value) {
		_absolute.insert(std::make_pair(type, value));
	}

	void createContainer() const {
		_ctx->addContainer(ContainerPtr(new Container(_name, _percentage, _absolute)));
	}

	inline const std::string& getName() const {
		return _name;
	}
};

}
