/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace video {

enum class PenAxis : uint8_t {
	Pressure, /**< Pen pressure.  Unidirectional: 0 to 1.0 */
	Xtilt,	  /**< Pen horizontal tilt angle.  Bidirectional: -90.0 to 90.0 (left-to-right). */
	Ytilt,	  /**< Pen vertical tilt angle.  Bidirectional: -90.0 to 90.0 (top-to-down). */
	Distance, /**< Pen distance to drawing surface.  Unidirectional: 0.0 to 1.0 */
	Rotation, /**< Pen barrel rotation.  Bidirectional: -180 to 179.9 (clockwise, 0 is facing up, -180.0 is facing
				 down). */
	Slider,	  /**< Pen finger wheel or slider (e.g., Airbrush Pen).  Unidirectional: 0 to 1.0 */
	TangentialPressure, /**< Pressure from squeezing the pen ("barrel pressure"). */
	Unknown,
	Max
};

}
