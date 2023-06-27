/**
 * @file
 */

#pragma once

namespace scenegraph {

enum class CoordinateSystem {
	XRightYUpZBack,
	XRightYBackZUp,
	XLeftYForwardZUp,
	Max,

	Vengi = XRightYUpZBack,
	MagicaVoxel = XLeftYForwardZUp,
	VXL = XRightYBackZUp
};

}
