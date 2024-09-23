/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>

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

static const glm::ivec3 arrayPathfinderFaces[6] = {glm::ivec3(0, 0, -1), glm::ivec3(0, 0, +1), glm::ivec3(0, -1, 0),
												   glm::ivec3(0, +1, 0), glm::ivec3(-1, 0, 0), glm::ivec3(+1, 0, 0)};

static const glm::ivec3 arrayPathfinderEdges[12] = {
	glm::ivec3(0, -1, -1), glm::ivec3(0, -1, +1), glm::ivec3(0, +1, -1), glm::ivec3(0, +1, +1),
	glm::ivec3(-1, 0, -1), glm::ivec3(-1, 0, +1), glm::ivec3(+1, 0, -1), glm::ivec3(+1, 0, +1),
	glm::ivec3(-1, -1, 0), glm::ivec3(-1, +1, 0), glm::ivec3(+1, -1, 0), glm::ivec3(+1, +1, 0)};

static const glm::ivec3 arrayPathfinderCorners[8] = {
	glm::ivec3(-1, -1, -1), glm::ivec3(-1, -1, +1), glm::ivec3(-1, +1, -1), glm::ivec3(-1, +1, +1),
	glm::ivec3(+1, -1, -1), glm::ivec3(+1, -1, +1), glm::ivec3(+1, +1, -1), glm::ivec3(+1, +1, +1)};

} // namespace voxel
