/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
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

/**
 * @brief Calculates the distance between two colors based on the given Distance function.
 */
float getDistance(RGBA rgba, RGBA rgba2, Distance d);
/**
 * @brief Calculates the distance between a color and HSB values.
 */
float getDistance(RGBA color, float hue, float saturation, float brightness);
/**
 * @brief HSB distance between two already-converted HSB triples (same weights as getDistance HSB).
 */
float getDistanceHSB(float hue1, float saturation1, float brightness1, float hue2, float saturation2,
					 float brightness2);

} // namespace color
