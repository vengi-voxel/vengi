/**
 * @file
 */

#pragma once

namespace noise {

/**
 * @brief Makes sure that the given double value is identical between platforms. A simple
 * cast wouldn't
 */
inline double ensure32BitRange(double n) {
	constexpr double c = (double)glm::pow(2, 30);
	if (n >= c) {
		return (2.0 * glm::fmod(n, c)) - c;
	}
	if (n <= -c) {
		return (2.0 * glm::fmod(n, c)) + c;
	}
	return n;
}

}
