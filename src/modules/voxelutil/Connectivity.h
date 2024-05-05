/**
 * @file
 */

#pragma once

namespace voxelutil {

/// The Connectivity of a voxel determines how many neighbours it has.
enum Connectivity {
	/// Each voxel has six neighbours, which are those sharing a face.
	SixConnected,
	/// Each voxel has 18 neighbours, which are those sharing a face or an edge.
	EighteenConnected,
	/// Each voxel has 26 neighbours, which are those sharing a face, edge, or corner.
	TwentySixConnected
};

} // namespace voxelutil
