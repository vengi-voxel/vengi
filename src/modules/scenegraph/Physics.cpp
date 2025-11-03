/**
 * @file
 */

#include "Physics.h"
#include "math/Axis.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/ext/scalar_constants.hpp>

namespace scenegraph {

void Physics::binarySearch(const CollisionNodes &nodes, KinematicBody &body, int axisIdx, float low, float high,
						   int iterations) const {
	for (int i = 0; i < iterations; i++) {
		if (glm::abs(high - low) < glm::epsilon<float>()) {
			// We have converged to a solution.
			break;
		}
		const float mid = 0.5f * (low + high);
		glm::vec3 pos = body.position;
		pos[axisIdx] = mid;

		if (checkCollision(nodes, pos, body)) {
			// Collision at the midpoint, so the actual collision is between low and mid.
			high = mid;
		} else {
			// No collision at the midpoint, so the actual collision is between mid and high.
			low = mid;
		}
	}
	// Move the body to the last known non-colliding position.
	body.position[axisIdx] = low;
	body.contactPoint[axisIdx] = high;
	// Stop movement on this axis.
	body.velocity[axisIdx] = 0.0f;
}

bool Physics::checkCollisionOnAxis(const CollisionNodes &nodes, KinematicBody &body, const glm::vec3 &nextPos,
								   math::Axis axis) const {
	const int axisIdx = math::getIndexForAxis(axis);
	const float targetPos = nextPos[axisIdx];
	const float curPos = body.position[axisIdx];
	const float length = targetPos - curPos;
	const float distance = glm::abs(length);
	// If there is no movement on this axis, there is no collision to check.
	if (distance < glm::epsilon<float>()) {
		return false;
	}

	// Test if we can move to the target position without a collision.
	glm::vec3 pos = body.position;
	pos[axisIdx] = targetPos;

	if (!checkCollision(nodes, pos, body)) {
		// No collision, so we can move the body to the target position.
		body.position[axisIdx] = targetPos;
		// we still need this for the contact point - but this is meaningless if there was no collision on any of the other axes
		body.contactPoint[axisIdx] = targetPos;
		return false;
	}

	// A collision was detected. We need to find the exact point of collision.
	// We use a binary search (bisection method) to find the closest non-colliding position.
	binarySearch(nodes, body, axisIdx, curPos, targetPos);
	return true;
}

bool Physics::checkCollision(const CollisionNodes &nodes, const glm::vec3 &nextBodyPos,
							 const KinematicBody &body) const {
	constexpr float epsilon = glm::epsilon<float>();
	const glm::vec3 &extents = body.extents;
	for (const CollisionNode &node : nodes) {
		const voxel::Region &region = node.volume->region();
		// Transform the body's position into the model space of the collision node.
		const glm::vec3 &pos = node.calcModelSpace(nextBodyPos);
		// Calculate the AABB of the body in the model space of the collision node.
		const glm::ivec3 &mins = glm::floor(pos - glm::vec3(extents.x + epsilon, epsilon, extents.z + epsilon));
		const glm::ivec3 &maxs = glm::floor(pos + extents);
		if (!region.containsPoint(mins) && !region.containsPoint(maxs)) {
			continue;
		}

		voxelutil::visitVolume(*node.volume, {mins, maxs}, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			return true;
		});
	}
	return false;
}

void Physics::applyFriction(KinematicBody &body, double deltaSeconds) const {
	if (!body.collidedY) {
		return;
	}
	// If the body is on the ground, apply friction.
	const float decay = glm::pow(body.frictionDecay, (float)deltaSeconds);
	body.velocity.x *= decay;
	body.velocity.z *= decay;

	if (glm::abs(body.velocity.x) < 0.01f) {
		body.velocity.x = 0.0f;
	}
	if (glm::abs(body.velocity.z) < 0.01f) {
		body.velocity.z = 0.0f;
	}
}

void Physics::update(double deltaSeconds, const CollisionNodes &nodes, KinematicBody &body, float gravity) const {
	// If there are no collision nodes, there is nothing to do.
	if (nodes.empty()) {
		return;
	}
	// Apply gravity to the body.
	body.velocity.y -= gravity * (float)deltaSeconds;
	body.contactPoint = {0.0f, 0.0f, 0.0f};
	// Calculate the next potential position of the body.
	const glm::vec3 nextPos = body.position + (body.velocity * (float)deltaSeconds);
	// Check for collision on each axis separately. This allows for sliding along walls.
	const bool collidedX = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::X);
	const bool collidedY = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::Y);
	const bool collidedZ = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::Z);
	const bool collisionChange = (body.collidedX != collidedX) || (body.collidedY != collidedY) || (body.collidedZ != collidedZ);
	const bool collided = (collidedX || collidedY || collidedZ);

	if (collisionChange && collided) {
		if (body.contactListener) {
			body.contactListener->onContact(body.contactPoint);
		}
	}

	if (body.collidedY && collidedY && body.velocity.y <= glm::epsilon<float>() && (!collidedX || !collidedZ)) {
		applyFriction(body, deltaSeconds);
	}
	body.collidedX = collidedX;
	body.collidedY = collidedY;
	body.collidedZ = collidedZ;
}

} // namespace scenegraph
