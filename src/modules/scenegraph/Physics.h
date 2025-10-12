/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "scenegraph/FrameTransform.h"

namespace voxel {
class RawVolume;
}

namespace scenegraph {

struct KinematicBody {
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 extents;
	bool onGround = false;
	float friction = 0.8f;
};

struct CollisionNode {
	CollisionNode(const voxel::RawVolume *v, const scenegraph::FrameTransform &t) : volume(v), transform(t) {
	}
	const voxel::RawVolume *volume;
	const scenegraph::FrameTransform transform;
};
using CollisionNodes = core::DynamicArray<CollisionNode>;

} // namespace scenegraph
