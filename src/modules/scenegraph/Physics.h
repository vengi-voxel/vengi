/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "math/Axis.h"
#include "scenegraph/FrameTransform.h"

namespace voxel {
class RawVolume;
}

namespace scenegraph {

struct ContactListener {
	virtual void onContact(const glm::vec3& point) = 0;
};

/**
 * @brief Represents a physical body that can move and collide with the environment.
 */
struct KinematicBody {
	glm::vec3 position{0.0f};
	glm::vec3 velocity{0.0f};
	// this should not be higher than 0.4998 - otherwise the collision detection can fail because the voxel cell would be off by one
	glm::vec3 extents{0.2f, 0.8f, 0.2f};
	glm::vec3 contactPoint{0.0f};
	bool collidedX = false;
	bool collidedY = false;
	bool collidedZ = false;
	float frictionDecay = 0.01f;
	ContactListener *contactListener = nullptr;
};

/**
 * @brief Represents a node in the scene graph that can be collided with.
 */
struct CollisionNode {
	CollisionNode(const voxel::RawVolume *v, const scenegraph::FrameTransform &t) : volume(v), transform(t) {
	}
	const voxel::RawVolume *volume;
	/**
	 * @brief The frame transform as returned by @c SceneGraph::transformForFrame()
	 */
	const scenegraph::FrameTransform transform;
};

/**
 * @brief A collection of collision nodes.
 * @sa CollisionNode
 */
using CollisionNodes = core::DynamicArray<CollisionNode>;

/**
 * @brief Handles physics simulation for kinematic bodies.
 *
 * This class provides methods to update the state of a @c KinematicBody, including its position and velocity,
 * based on gravity and collisions with the environment.
 *
 * https://www.youtube.com/watch?v=3lBYVSplAuo
 */
class Physics {
private:
	/**
	 * @brief Performs a binary search to find the precise collision point along an axis.
	 *
	 * This method is used to refine the position of a @c KinematicBody after a collision is detected.
	 * It iteratively narrows down the search space to find the boundary between a collided and non-collided state.
	 *
	 * @param[in] nodes The collision nodes to check against.
	 * @param[in,out] body The kinematic body being moved. Its position will be updated to the found collision point.
	 * @param[in] axisIdx The index of the axis (0 for X, 1 for Y, 2 for Z) to perform the search on.
	 * @param[in] low The lower bound of the search range along the axis.
	 * @param[in] high The upper bound of the search range along the axis.
	 * @param[in] iterations The number of iterations to perform for the binary search. More iterations lead to higher
	 * precision.
	 */
	void binarySearch(const CollisionNodes &nodes, KinematicBody &body, int axisIdx, float low, float high,
					  int iterations = 10) const;
	/**
	 * @brief Checks for collision on a single axis and resolves it.
	 * @param[in] nodes The collision nodes to check against.
	 * @param[in,out] body The kinematic body to check for collision.
	 * @param[in] nextPos The next potential position of the body.
	 * @param[in] axis The axis to check for collision.
	 * @return @c true if a collision occurred on the given axis, @c false otherwise.
	 */
	bool checkCollisionOnAxis(const CollisionNodes &nodes, KinematicBody &body, const glm::vec3 &nextPos,
							  math::Axis axis) const;
	/**
	 * @brief Checks for collision at a given position.
	 * @param[in] nodes The collision nodes to check against.
	 * @param[in] pos The position to check for collision.
	 * @param[in] body The kinematic body to check for collision.
	 * @return @c true if a collision occurred at the given position, @c false otherwise.
	 */
	bool checkCollision(const CollisionNodes &nodes, const glm::vec3 &pos, const KinematicBody &body) const;

	/**
	 * @brief Applies friction to the kinematic body's velocity.
	 *
	 * This is typically called when the body is on the ground to simulate friction, slowing it down over time.
	 *
	 * @param[in,out] body The kinematic body to apply friction to. Its velocity will be modified.
	 * @param[in] deltaSeconds The time elapsed since the last frame.
	 */
	void applyFriction(KinematicBody &body, double deltaSeconds) const;

public:
	/**
	 * @brief Updates the state of a kinematic body.
	 * @param[in] deltaSeconds The time elapsed since the last update in seconds.
	 * @param[in] nodes The collision nodes to check against.
	 * @param[in,out] body The kinematic body to update.
	 * @param[in] gravity The gravity to apply to the body.
	 */
	void update(double deltaSeconds, const CollisionNodes &nodes, KinematicBody &body, float gravity) const;
};

} // namespace scenegraph
