/**
 * @file
 */

#pragma once

#include <tb_config.h>

#include <stdint.h>
#include <string.h>

namespace tb {

template <class T> T Max(const T &left, const T &right) {
	return left > right ? left : right;
}

template <class T> T Min(const T &left, const T &right) {
	return left < right ? left : right;
}

template <class T> T Abs(const T &value) {
	return value < 0 ? -value : value;
}

template <class T> T Clamp(const T &value, const T &min, const T &max) {
	return (value > max) ? max : ((value < min) ? min : value);
}

/** Returns value clamped to min and max. If max is greater than min,
	max will be clipped to min. */
template <class T> T ClampClipMax(const T &value, const T &min, const T &max) {
	return (value > max) ? (max > min ? max : min) : ((value < min) ? min : value);
}

} // namespace tb
