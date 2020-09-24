/**
 * @file
 */

#pragma once

#include "Container.h"

#ifdef _WIN32
#undef max
#endif

namespace attrib {

/**
 * @brief ShadowAttributes can be used on the client side to just shadow the server side state.
 *
 * There is no max calculation done here, but just taken from the server. This class is not thread safe.
 *
 * @sa Attributes
 * @ingroup Attributes
 */
class ShadowAttributes {
protected:
	Values _current;
	Values _max;
public:
	bool update(double /*deltaFrameSeconds*/) {
		return true;
	}

	inline double setMax(Type type, double value) {
		_max[core::enumVal(type)] = value;
		return value;
	}

	inline double setCurrent(Type type, double value) {
		_current[core::enumVal(type)] = value;
		return value;
	}

	inline double current(Type type) const {
		return _current[core::enumVal(type)];
	}

	inline double max(Type type) const {
		return _max[core::enumVal(type)];
	}
};

}
