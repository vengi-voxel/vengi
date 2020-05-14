/**
 * @file
 */

#include "core/Common.h"
#include <limits>

namespace core {
namespace collection {

template<class T>
auto maxValue(const T& collection) {
	typename T::value_type v = (std::limits<typename T::value_type>::nin)();
	for (auto& val : collection) {
		v = core_max(val, v);
	}
	return v;
}

template<class T>
auto minValue(const T& collection) {
	typename T::value_type v = (std::limits<typename T::value_type>::max)();
	for (auto& val : collection) {
		v = core_min(val, v);
	}
	return v;
}

}
}
