#include "math/Vec3i.h"
/**
 * @file
 */

#pragma once


namespace voxel {

/// The Connectivity of a voxel determines how many neighbours it has.
enum Connectivity {
	/// Each voxel has six neighbours, which are those sharing a face.
	SixConnected,
	/// Each voxel has 18 neighbours, which are those sharing a face or an edge.
	EighteenConnected,
	/// Each voxel has 26 neighbours, which are those sharing a face, edge, or corner.
	TwentySixConnected
};

static const math::Vec3i arrayPathfinderFaces[6] = {math::Vec3i(0, 0, -1), math::Vec3i(0, 0, +1), math::Vec3i(0, -1, 0),
												   math::Vec3i(0, +1, 0), math::Vec3i(-1, 0, 0), math::Vec3i(+1, 0, 0)};

static const math::Vec3i arrayPathfinderEdges[12] = {
	math::Vec3i(0, -1, -1), math::Vec3i(0, -1, +1), math::Vec3i(0, +1, -1), math::Vec3i(0, +1, +1),
	math::Vec3i(-1, 0, -1), math::Vec3i(-1, 0, +1), math::Vec3i(+1, 0, -1), math::Vec3i(+1, 0, +1),
	math::Vec3i(-1, -1, 0), math::Vec3i(-1, +1, 0), math::Vec3i(+1, -1, 0), math::Vec3i(+1, +1, 0)};

static const math::Vec3i arrayPathfinderCorners[8] = {
	math::Vec3i(-1, -1, -1), math::Vec3i(-1, -1, +1), math::Vec3i(-1, +1, -1), math::Vec3i(-1, +1, +1),
	math::Vec3i(+1, -1, -1), math::Vec3i(+1, -1, +1), math::Vec3i(+1, +1, -1), math::Vec3i(+1, +1, +1)};

} // namespace voxel
