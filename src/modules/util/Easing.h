/**
 * @file
 */

#pragma once

#include <glm/common.hpp>
#include <glm/exponential.hpp>

namespace util {
/**
 * @brief Other useful tween and easing functions can be found here
 * http://robertpenner.com/easing/penner_chapter7_tweening.pdf
 */
namespace easing {

inline constexpr double linear(double current, double start, double end) {
	return (current - start) / (end - start);
}

inline double full(double current, double start, double end) {
	return glm::round(linear(current, start, end));
}

inline constexpr double quadInOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	if (v < 0.5) {
		return 2.0 * v * v;
	}
	return -1.0 + (4.0 - 2.0 * v) * v;
}

inline constexpr double quadOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	return v * (2.0 - v);
}

inline double quadIn(double current, double start, double end) {
	return glm::pow(linear(current, start, end), 2.0);
}

inline double cubicIn(double current, double start, double end) {
	return glm::pow(linear(current, start, end), 3.0);
}

inline double cubicOut(double current, double start, double end) {
	const double v = linear(current, start, end) - 1.0;
	return glm::pow(v, 3.0) + 1.0;
}

inline double cubicInOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	if (v < 0.5) {
		return glm::pow(v, 3.0) * 4.0;
	}
	const double v1 = (2.0 * v - 2.0);
	return (v - 1.0) * glm::pow(v1, 2.0) + 1.0f;
}

} // namespace easing
} // namespace util
