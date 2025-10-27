/**
 * @file
 */

#pragma once

#include <glm/common.hpp>
#include <glm/exponential.hpp>

namespace util {
/**
 * @brief Other useful tween and easing functions can be found here
 * @li http://robertpenner.com/easing/penner_chapter7_tweening.pdf
 * @li https://iquilezles.org/articles/functions/
 * @li https://iquilezles.org/articles/smoothsteps/
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

/**
 * Bezier cubic formula: B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
 * where P0 = (0, 0), P1 = (cp1x, cp1y), P2 = (cp2x, cp2y), P3 = (1, 1)
 *
 * @sa https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Cubic_B%C3%A9zier_curves
 */
inline double cubicBezier(double current, double start, double end, double cp1y, double cp2y) {
	const double t = linear(current, start, end);
	const double omt = 1.0 - t;
	const double omt2 = omt * omt;
	const double t2 = t * t;
	const double t3 = t2 * t;

	return 3.0 * omt2 * t * cp1y + 3.0 * omt * t2 * cp2y + t3;
}

/**
 * Catmull-Rom spline interpolation between p1 and p2, with p0 and p3 as control points
 * Uses the standard formulation with tangent vectors: m0 = (p2 - p0) / 2, m1 = (p3 - p1) / 2
 *
 * @sa https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Catmull%E2%80%93Rom_spline
 */
inline double catmullRom(double p0, double p1, double p2, double p3, double t) {
	const double v0 = (p2 - p0) * 0.5;
	const double v1 = (p3 - p1) * 0.5;
	const double t2 = t * t;
	const double t3 = t * t2;
	return (2.0 * p1 - 2.0 * p2 + v0 + v1) * t3 + (-3.0 * p1 + 3.0 * p2 - 2.0 * v0 - v1) * t2 + v0 * t + p1;
}

} // namespace easing
} // namespace util
