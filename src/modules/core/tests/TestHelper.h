/**
 * @file
 */

#pragma once

#include <gtest/gtest.h>
#include "core/String.h"
#include "core/Path.h"
#include "core/collection/DynamicArray.h"

namespace core {

inline std::ostream &operator<<(::std::ostream &os, const String &dt) {
	return os << dt.c_str();
}

inline std::ostream &operator<<(::std::ostream &os, const Path &dt) {
	return os << dt.c_str();
}

inline std::ostream &operator<<(::std::ostream &os, const DynamicArray<String> &dt) {
	for (auto i = dt.begin(); i != dt.end();) {
		os << *i;
		if (++i != dt.end()) {
			os << ", ";
		}
	}
	return os;
}

}

template<class T, size_t SIZE>
core::String toString(const core::DynamicArray<T, SIZE>& v) {
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
