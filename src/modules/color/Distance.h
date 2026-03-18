/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace color {

enum class Distance : uint8_t {
	// computational less expensive distance function
	Approximation,
	// hue, saturation, brightness distance function
	HSB,
	Max
};

/**
 * @name Approximation distance constants
 * @brief Constants for the low-cost color distance approximation from https://www.compuphase.com/cmetric.htm
 * @{
 */
/** Minimum distance (identical colors) */
static constexpr float ApproximationDistanceMin = 0.0f;
/** Maximum distance (black to white) */
static constexpr float ApproximationDistanceMax = 584970.0f;
/** Suggested tight threshold for very similar colors */
static constexpr float ApproximationDistanceTight = 5000.0f;
/** Suggested moderate threshold for similar colors */
static constexpr float ApproximationDistanceModerate = 50000.0f;
/** Suggested loose threshold for broadly similar colors */
static constexpr float ApproximationDistanceLoose = 150000.0f;
/** @} */

} // namespace color
