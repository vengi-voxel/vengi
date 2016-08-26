/**
 * @file
 */

#pragma once

#include "Container.h"

namespace attrib {

/**
 * @brief ShadowAttributes can be used on the client side to just shadow the server side state.
 *
 * There is no max calculation done here, but just taken from the server. This class is not thread safe.
 */
class ShadowAttributes {
protected:
	Values _current;
	Values _max;
public:
	bool onFrame(long /*dt*/) {
		return true;
	}

	inline void setMax(Types type, double value) {
		_max[type] = value;
	}

	double setCurrent(Types type, double value) {
		_current[type] = value;
		return value;
	}

	double getCurrent(Types type) const {
		auto i = _current.find(type);
		if (i == _current.end()) {
			return 0.0;
		}
		return i->second;
	}

	double getMax(Types type) const {
		auto i = _max.find(type);
		if (i == _max.end()) {
			return 0.0;
		}
		return i->second;
	}
};

}
