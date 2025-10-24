/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace math {

enum class CoordinateSystem : uint8_t {
	Vengi,
	MagicaVoxel,
	VXL,
	DirectX,
	OpenGL,
	Maya,
	Autodesk3dsmax,

	Max
};

}
