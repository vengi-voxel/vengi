/**
 * @file
 */

#include "Physics.h"
#include "core/TraceScoped.h"
#include "math/Axis.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "math/Mat3f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"
#include "math/Scalar.h"

namespace scenegraph {

bool Physics::consumeWork() const {
	if (_work == _workLimit) {
		_exhausted = true;
		return false;
	}
	++_work;
	return true;
}

void Physics::binarySearch(const CollisionNodes &nodes, KinematicBody &body, int axisIdx, float low, float high,
						   int iterations) const {
	core_trace_scoped(BinarySearch);
	for (int i = 0; i < iterations; i++) {
		if (math::abs(high - low) < math::epsilonf) {
			// We have converged to a solution.
			break;
		}
		const float mid = 0.5f * (low + high);
		math::Vec3f pos = body.position;
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

bool Physics::checkCollisionOnAxis(const CollisionNodes &nodes, KinematicBody &body, const math::Vec3f &nextPos,
								   math::Axis axis) const {
	core_trace_scoped(CheckCollisionOnAxis);
	const int axisIdx = math::getIndexForAxis(axis);
	const float targetPos = nextPos[axisIdx];
	const float curPos = body.position[axisIdx];
	const float length = targetPos - curPos;
	const float distance = math::abs(length);
	// If there is no movement on this axis, there is no collision to check.
	if (distance < math::epsilonf) {
		return false;
	}

	// Test if we can move to the target position without a collision.
	math::Vec3f pos = body.position;
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

bool Physics::checkCollision(const CollisionNodes &nodes, const math::Vec3f &nextBodyPos,
							 const KinematicBody &body) const {
	core_trace_scoped(CheckCollision);
	const float epsilon = math::epsilonf;
	const math::Vec3f &extents = body.extents;
	for (const CollisionNode &node : nodes) {
		if (!consumeWork()) {
			return true;
		}
		if (!node.volume) {
			continue;
		}
		const voxel::Region &region = node.volume->region();
		// Transform the entire body AABB, including scale and rotation of the voxel model.
		const math::Vec3f lower = nextBodyPos - math::Vec3f(extents.x + epsilon, epsilon, extents.z + epsilon);
		const math::Vec3f upper = nextBodyPos + extents;
		const math::Vec3f half = (upper - lower) * 0.5f;
		const math::Vec3f center = node.calcModelSpace((lower + upper) * 0.5f);
		const math::Mat3f linear(node.worldToModel);
		const math::Vec3f modelHalf = math::abs(linear[0]) * half.x + math::abs(linear[1]) * half.y + math::abs(linear[2]) * half.z;
		const math::Vec3i mins = math::floor(center - modelHalf);
		const math::Vec3i maxs = math::floor(center + modelHalf);
		if (!voxel::intersects(region, voxel::Region(mins, maxs))) {
			continue;
		}

		// Create a sampler for the volume of the collision node.
		voxel::RawVolume::Sampler sampler(node.volume);
		sampler.setPosition(mins);
		// Iterate over all voxels in the AABB and check for collision.
		for (int z = mins.z; z <= maxs.z; z++) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = mins.y; y <= maxs.y; y++) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int x = mins.x; x <= maxs.x; x++) {
					if (!consumeWork()) {
						return true;
					}
					if (voxel::isBlocked(sampler3.voxel().getMaterial())) {
						return true;
					}
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	}
	return false;
}

void Physics::applyFriction(KinematicBody &body, double deltaSeconds) const {
	// If the body is on the ground, apply friction.
	const float decay = math::pow(body.frictionDecay, (float)deltaSeconds);
	body.velocity.x *= decay;
	body.velocity.z *= decay;

	if (math::abs(body.velocity.x) < 0.01f) {
		body.velocity.x = 0.0f;
	}
	if (math::abs(body.velocity.z) < 0.01f) {
		body.velocity.z = 0.0f;
	}
}

bool Physics::tryStepUp(const CollisionNodes &nodes, KinematicBody &body, const math::Vec3f &desiredHorizontalPos) const {
	core_trace_scoped(TryStepUp);
	// Only allow stepping up when grounded
	if (!body.collidedY) {
		return false;
	}

	// Check if there's horizontal movement being attempted
	const math::Vec2f horizontalDelta(desiredHorizontalPos.x - body.position.x, desiredHorizontalPos.z - body.position.z);
	const float horizontalDistance = math::length(horizontalDelta);
	if (horizontalDistance < math::epsilonf) {
		return false;
	}

	const float maxStepHeight = body.extents.y;
	constexpr float stepIncrement = 0.1f;
	const int maxSteps = static_cast<int>(math::ceil(maxStepHeight / stepIncrement));

	for (int i = 1; i <= maxSteps; ++i) {
		const float testStepHeight = i * stepIncrement;

		math::Vec3f testPos = body.position;
		testPos.y += testStepHeight;
		testPos.x = desiredHorizontalPos.x;
		testPos.z = desiredHorizontalPos.z;

		if (!checkCollision(nodes, testPos, body)) {
			bool foundGround = false;
			for (float groundCheckDelta = 0.1f; groundCheckDelta <= testStepHeight + 0.5f; groundCheckDelta += 0.1f) {
				math::Vec3f groundCheckPos = testPos;
				groundCheckPos.y -= groundCheckDelta;

				if (checkCollision(nodes, groundCheckPos, body)) {
					foundGround = true;
					break;
				}
			}

			if (foundGround) {
				body.position = testPos;
				body.collidedX = false;
				body.collidedZ = false;
				return true;
			}
		}
	}

	return false;
}

void Physics::update(double deltaSeconds, const CollisionNodes &nodes, KinematicBody &body, float gravity) const {
	core_trace_scoped(PhysicsUpdate);
	// If there are no collision nodes, there is nothing to do.
	if (nodes.empty()) {
		return;
	}
	// Apply gravity to the body.
	body.velocity.y -= gravity * (float)deltaSeconds;
	body.contactPoint = {0.0f, 0.0f, 0.0f};
	// Calculate the next potential position and velocity of the body using explicit euler integration.
	// Xn+1 = Xn + Vn * dt
	// Vn+1 = Vn + An * dt
	const math::Vec3f nextPos = body.position + (body.velocity * (float)deltaSeconds);

	// First, handle vertical collision (Y axis)
	const bool collidedY = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::Y);

	bool steppedUp = false;
	const bool hasHorizontalVelocity = math::abs(body.velocity.x) > math::epsilonf || math::abs(body.velocity.z) > math::epsilonf;

	if (collidedY && hasHorizontalVelocity) {
		math::Vec3f testPos = body.position;
		testPos.x = nextPos.x;
		testPos.z = nextPos.z;

		if (checkCollision(nodes, testPos, body)) {
			steppedUp = tryStepUp(nodes, body, nextPos);
		}
	}

	bool collidedX = false;
	bool collidedZ = false;

	if (!steppedUp) {
		collidedX = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::X);
		collidedZ = checkCollisionOnAxis(nodes, body, nextPos, math::Axis::Z);
	}

	const bool collisionChange = (body.collidedX != collidedX) || (body.collidedY != collidedY) || (body.collidedZ != collidedZ);
	const bool collided = (collidedX || collidedY || collidedZ);

	if (collisionChange && collided && !steppedUp) {
		if (body.contactListener) {
			body.contactListener->onContact(body.contactPoint);
		}
	}

	if (body.collidedY && collidedY && body.velocity.y <= math::epsilonf && (!collidedX || !collidedZ)) {
		applyFriction(body, deltaSeconds);
	}
	body.collidedX = collidedX;
	body.collidedY = collidedY;
	body.collidedZ = collidedZ;
}

} // namespace scenegraph
