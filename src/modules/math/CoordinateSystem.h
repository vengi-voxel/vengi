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
	SceneKit, // same axes as OpenGL/vengi: right-handed, Y-up, +Z toward viewer
	Autodesk3dsmax,

	Max
};

}
