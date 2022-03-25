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

constexpr double linear(double current, double start, double end) {
	return (current - start) / (end - start);
}

constexpr double full(double current, double start, double end) {
	return glm::round(linear(current, start, end));
}

constexpr double quadInOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	if (v < 0.5) {
		return 2.0 * v * v;
	}
	return -1.0 + (4.0 - 2.0 * v) * v;
}

constexpr double quadOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	return v * (2.0 - v);
}

constexpr double quadIn(double current, double start, double end) {
	return glm::pow(linear(current, start, end), 2.0);
}

constexpr double cubicIn(double current, double start, double end) {
	return glm::pow(linear(current, start, end), 3.0);
}

constexpr double cubicOut(double current, double start, double end) {
	const double v = linear(current, start, end) - 1.0;
	return glm::pow(v, 3.0) + 1.0;
}

constexpr double cubicInOut(double current, double start, double end) {
	const double v = linear(current, start, end);
	if (v < 0.5) {
		return glm::pow(v, 3.0) * 4.0;
	}
	const double v1 = (2.0 * v - 2.0);
	return (v - 1.0) * glm::pow(v1, 2.0) + 1.0f;
}

} // namespace easing
} // namespace util
