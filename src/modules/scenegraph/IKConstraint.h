/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include <glm/vec2.hpp>

namespace scenegraph {

/**
 * @brief Inverse kinematics constraint for a scene graph node
 *
 * Defines the IK parameters that constrain how a node can move relative to its IK chain.
 * The effector is an arbitrary target node in the scene graph that the IK solver tries to reach
 * (not necessarily the parent node, which defines the kinematic chain hierarchy).
 *
 * @ingroup SceneGraph
 */
struct IKConstraint {
	IKConstraint();
	/**
	 * @brief The node id of the IK end-effector target. This is the node the IK chain tries
	 * to reach and can be any node in the scene graph or @c InvalidNodeId if no effector is assigned.
	 */
	int effectorNodeId = 0;
	/** @brief Minimum roll angle in radians */
	float rollMin = 0.0f;
	/** @brief Maximum roll angle in radians */
	float rollMax = 0.0f;
	/** @brief Whether this IK constraint is visible in the editor */
	bool visible = true;
	/** @brief Whether this node acts as an anchor (fixed point) in the IK chain - usually the leg and arm nodes are no anchor nodes */
	bool anchor = false;

	/**
	 * @brief Defines a swing limit as a circle on the constraint cone surface
	 */
	struct RadiusConstraint {
		/** @brief Center of the swing limit circle (polar coordinates on the cone) */
		glm::vec2 center {0.0f, 0.0f}; // yaw, pitch
		/** @brief Radius of the allowed swing region */
		float radius = 0.0f;
	};
	/** @brief Array of swing constraints that limit the angular range of the joint */
	core::DynamicArray<RadiusConstraint> swingLimits;
};

} // namespace scengraph
