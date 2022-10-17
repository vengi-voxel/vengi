/**
 * @file
 */

#pragma once

#include <gtest/gtest.h>
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace core {

inline std::ostream &operator<<(::std::ostream &os, const String &dt) {
	return os << dt.c_str();
}

}

template<class T>
core::String toString(const core::DynamicArray<T>& v) {
	core::String str;
	str.reserve(4096);
	for (auto i = v.begin(); i != v.end();) {
		str += "'";
		str += *i;
		str += "'";
		if (++i != v.end()) {
			str += ", ";
		}
	}
	return str;
}
