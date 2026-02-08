/**
 * @file
 */

#pragma once

#include "IKConstraint.h"
#include "SceneGraphAnimation.h"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace scenegraph {

class SceneGraph;
class SceneGraphNode;

/**
 * @brief CCD (Cyclic Coordinate Descent) based Inverse Kinematics solver
 *
 * Solves an IK chain from a node up to an anchor node so that the chain's
 * end-effector reaches a target position. The solver respects the IKConstraint
 * settings (roll limits, swing limits, anchor flag) on each joint in the chain.
 *
 * @ingroup SceneGraph
 */
class IKSolver {
public:
	/**
	 * @brief Maximum number of CCD iterations
	 */
	static constexpr int MaxIterations = 20;
	/**
	 * @brief Distance threshold to consider the target reached
	 */
	static constexpr float DistanceThreshold = 0.01f;

	/**
	 * @brief Solve the IK chain for the given node
	 *
	 * Walks up the hierarchy from @p node to the anchor node and applies CCD iterations
	 * to bring the effector position closer to the target. Each joint's local orientation
	 * is clamped to its IKConstraint limits.
	 *
	 * @param sceneGraph The scene graph containing all nodes
	 * @param node The node whose IK chain should be solved (must have an IKConstraint with a valid effector)
	 * @param frameIdx The frame index to operate on
	 * @return @c true if the solver converged or made progress, @c false if the node has no valid IK setup
	 */
	static bool solve(SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx);

	/**
	 * @brief Clamp a local orientation to the constraint limits defined in the IKConstraint
	 *
	 * Applies roll (twist) limits and swing cone limits.
	 *
	 * @param localOrientation The orientation to constrain
	 * @param constraint The IK constraint defining the limits
	 * @return The clamped orientation
	 */
	static glm::quat clampOrientation(const glm::quat &localOrientation, const IKConstraint &constraint);
};

} // namespace scenegraph

