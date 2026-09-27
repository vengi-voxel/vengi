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
	Unity, // same axes as DirectX: left-handed, Y-up, Z-forward
	OpenGL,
	Maya,
	Autodesk3dsmax,

	Max
};

}
