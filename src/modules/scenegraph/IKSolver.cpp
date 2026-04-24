/**
 * @file
 */

#include "IKSolver.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace scenegraph {

/**
 * @brief Convert a swing limit's polar center (yaw, pitch) to a 3D unit direction vector.
 * Applies rotateX(yaw) then rotateZ(pitch) to the Y-up vector.
 */
static glm::vec3 coneDirection(const glm::vec2 &center) {
	const float cx = glm::cos(center.x), sx = glm::sin(center.x);
	const float cz = glm::cos(center.y), sz = glm::sin(center.y);
	return glm::vec3(-sz * cx, cz * cx, sx);
}

/**
 * @brief Rotate unit vector @p v toward @p target by @p angle radians.
 */
static glm::vec3 rotateToward(const glm::vec3 &v, const glm::vec3 &target, float angle) {
	const glm::vec3 axis = glm::cross(v, target);
	if (glm::length2(axis) < 0.0001f) {
		return v;
	}
	return glm::normalize(glm::angleAxis(angle, glm::normalize(axis)) * v);
}

/**
 * @brief Build a swing quaternion that rotates Y-axis to the given direction.
 */
static glm::quat swingFromDirection(const glm::vec3 &dir) {
	const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
	const glm::vec3 axis = glm::cross(yAxis, dir);
	if (glm::length2(axis) < 0.0001f) {
		return glm::quat_identity<float, glm::defaultp>();
	}
	const float angle = glm::acos(glm::clamp(glm::dot(yAxis, dir), -1.0f, 1.0f));
	return glm::angleAxis(angle, glm::normalize(axis));
}

/**
 * @brief Check if @p input lies in the tangent path region between two consecutive cones
 * on the unit sphere.
 *
 * Between consecutive cones A and B, there is a smooth allowed region defined by tangent
 * circles. The tangent circle radius = (PI - radiusA - radiusB) / 2, which is half the
 * angular gap between the two cone boundaries along the great arc connecting them.
 *
 * @param[out] result The clamped direction if the point is in the path region
 * @return true if the point is in the path region (result is set)
 */
static bool checkConePairPath(const glm::vec3 &coneA, float radiusA, const glm::vec3 &coneB, float radiusB,
							  const glm::vec3 &input, glm::vec3 &result) {
	const float tRadius = (glm::pi<float>() - (radiusA + radiusB)) / 2.0f;
	if (tRadius <= 0.0f) {
		return false;
	}

	const glm::vec3 arcNormal = glm::cross(coneA, coneB);
	if (glm::length2(arcNormal) < 0.0001f) {
		return false;
	}
	const glm::vec3 normArc = glm::normalize(arcNormal);

	// Compute two tangent circle centers (one on each side of the great arc)
	const glm::vec3 tan1 = glm::normalize(glm::angleAxis(radiusA + tRadius, normArc) * coneA);
	const glm::vec3 tan2 = glm::normalize(glm::angleAxis(-(radiusA + tRadius), normArc) * coneA);

	// Determine which side of the great arc the input is on
	const float side = glm::dot(input, glm::cross(coneA, coneB));
	const glm::vec3 &tanCenter = (side < 0.0f) ? tan1 : tan2;

	// Check if input is in the triangle region bounded by coneA, tanCenter, coneB
	// using cross-product half-plane tests
	const glm::vec3 edge1 = (side < 0.0f) ? glm::cross(coneA, tanCenter) : glm::cross(tanCenter, coneA);
	const glm::vec3 edge2 = (side < 0.0f) ? glm::cross(tanCenter, coneB) : glm::cross(coneB, tanCenter);

	if (glm::dot(input, edge1) <= 0.0f || glm::dot(input, edge2) <= 0.0f) {
		return false;
	}

	// In the path region. Check if inside the tangent circle (= out of bounds, needs clamping)
	if (glm::dot(input, tanCenter) > glm::cos(tRadius)) {
		result = glm::normalize(rotateToward(tanCenter, input, tRadius));
	} else {
		result = input;
	}
	return true;
}

glm::quat IKSolver::clampOrientation(const glm::quat &localOrientation, const IKConstraint &constraint) {
	const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
	const glm::vec3 rotatedAxis = localOrientation * yAxis;

	glm::quat swing;
	glm::quat twist;

	if (glm::abs(glm::dot(yAxis, rotatedAxis)) > 0.9999f) {
		twist = localOrientation;
		swing = glm::quat_identity<float, glm::defaultp>();
	} else {
		const glm::vec3 projection =
			yAxis * glm::dot(glm::vec3(localOrientation.x, localOrientation.y, localOrientation.z), yAxis);
		twist = glm::normalize(glm::quat(localOrientation.w, projection));
		swing = localOrientation * glm::conjugate(twist);
	}

	// Clamp twist to [rollMin, rollMax] using [0, 2*PI] convention
	float twistAngle = 2.0f * glm::atan(glm::length(glm::vec3(twist.x, twist.y, twist.z)), twist.w);
	if (twistAngle < 0.0f) {
		twistAngle += glm::two_pi<float>();
	}
	if (twistAngle < constraint.rollMin || twistAngle > constraint.rollMax) {
		const float distToMin = glm::min(glm::abs(twistAngle - constraint.rollMin),
										 glm::two_pi<float>() - glm::abs(twistAngle - constraint.rollMin));
		const float distToMax = glm::min(glm::abs(twistAngle - constraint.rollMax),
										 glm::two_pi<float>() - glm::abs(twistAngle - constraint.rollMax));
		twistAngle = (distToMin <= distToMax) ? constraint.rollMin : constraint.rollMax;
	}
	twist = glm::angleAxis(twistAngle, yAxis);

	// Clamp swing: per-cone containment + inter-cone path interpolation
	const int numCones = (int)constraint.swingLimits.size();
	if (numCones > 0) {
		const glm::vec3 swingDir = glm::normalize(swing * yAxis);

		// Step 1: Check if inside any individual cone
		bool inBounds = false;
		for (int i = 0; i < numCones; ++i) {
			if (glm::dot(swingDir, coneDirection(constraint.swingLimits[i].center)) >=
				glm::cos(constraint.swingLimits[i].radius)) {
				inBounds = true;
				break;
			}
		}

		// Step 2: Check inter-cone path regions between consecutive cones
		if (!inBounds && numCones > 1) {
			for (int i = 0; i < numCones - 1; ++i) {
				const auto &limA = constraint.swingLimits[i];
				const auto &limB = constraint.swingLimits[i + 1];
				glm::vec3 pathResult;
				if (checkConePairPath(coneDirection(limA.center), limA.radius, coneDirection(limB.center), limB.radius,
									  swingDir, pathResult)) {
					inBounds = true;
					// pathResult == swingDir means already in bounds, otherwise clamped
					if (glm::distance(pathResult, swingDir) > 0.0001f) {
						swing = swingFromDirection(pathResult);
					}
					break;
				}
			}
		}

		// Step 3: If still out of bounds, clamp to the closest cone boundary
		if (!inBounds) {
			float bestDot = -2.0f;
			int bestCone = 0;
			for (int i = 0; i < numCones; ++i) {
				const float d = glm::dot(swingDir, coneDirection(constraint.swingLimits[i].center));
				if (d > bestDot) {
					bestDot = d;
					bestCone = i;
				}
			}
			const auto &limit = constraint.swingLimits[bestCone];
			swing = swingFromDirection(rotateToward(coneDirection(limit.center), swingDir, limit.radius));
		}
	}

	return swing * twist;
}

bool IKSolver::solve(SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx) {
	const IKConstraint *constraint = node.ikConstraint();
	if (constraint == nullptr) {
		return false;
	}

	const int effectorNodeId = constraint->effectorNodeId;
	if (effectorNodeId == InvalidNodeId) {
		return false;
	}

	if (!sceneGraph.hasNode(effectorNodeId)) {
		return false;
	}

	// Build the chain from the node up to the anchor
	core::DynamicArray<int> chain;
	chain.push_back(node.id());

	int current = node.parent();
	while (current != InvalidNodeId && sceneGraph.hasNode(current)) {
		SceneGraphNode &currentNode = sceneGraph.node(current);
		chain.push_back(current);
		if (currentNode.isIKAnchor() || currentNode.isRootNode()) {
			break;
		}
		current = currentNode.parent();
	}

	if (chain.size() < 2) {
		return false;
	}

	// The target position is the world position of the effector node
	const SceneGraphNode &effectorNode = sceneGraph.node(effectorNodeId);
	const KeyFrameIndex effectorKeyFrameIdx = effectorNode.keyFrameForFrame(frameIdx);
	const glm::vec3 targetPos = effectorNode.transform(effectorKeyFrameIdx).worldTranslation();

	// Get the end-effector position (the node itself)
	for (int iter = 0; iter < MaxIterations; ++iter) {
		// Check if we've reached the target
		const KeyFrameIndex nodeKeyFrameIdx = node.keyFrameForFrame(frameIdx);
		const glm::vec3 endEffectorPos = node.transform(nodeKeyFrameIdx).worldTranslation();
		const float dist = glm::distance(endEffectorPos, targetPos);
		if (dist < DistanceThreshold) {
			return true;
		}

		// Iterate from the node's parent up through the chain (skip index 0 which is the end-effector itself)
		for (size_t i = 1; i < chain.size(); ++i) {
			const int jointId = chain[i];
			SceneGraphNode &jointNode = sceneGraph.node(jointId);
			const KeyFrameIndex jointKeyFrameIdx = jointNode.keyFrameForFrame(frameIdx);
			SceneGraphTransform &jointTransform = jointNode.transform(jointKeyFrameIdx);

			// Current positions
			const glm::vec3 jointPos = jointTransform.worldTranslation();

			// Recompute end effector position after previous joint adjustments
			const KeyFrameIndex currentNodeKeyFrameIdx = node.keyFrameForFrame(frameIdx);
			const glm::vec3 currentEndPos = node.transform(currentNodeKeyFrameIdx).worldTranslation();

			// Vectors from joint to end-effector and target
			glm::vec3 toEnd = currentEndPos - jointPos;
			glm::vec3 toTarget = targetPos - jointPos;

			const float toEndLen = glm::length(toEnd);
			const float toTargetLen = glm::length(toTarget);

			if (toEndLen < 0.0001f || toTargetLen < 0.0001f) {
				continue;
			}

			toEnd /= toEndLen;
			toTarget /= toTargetLen;

			// Calculate the rotation needed
			const float dotProduct = glm::clamp(glm::dot(toEnd, toTarget), -1.0f, 1.0f);
			if (dotProduct > 0.9999f) {
				continue; // Already aligned
			}

			const float angle = glm::acos(dotProduct);
			glm::vec3 rotAxis = glm::cross(toEnd, toTarget);

			if (glm::length2(rotAxis) < 0.0001f) {
				continue;
			}
			rotAxis = glm::normalize(rotAxis);

			// Create the rotation in world space
			const glm::quat worldRotation = glm::angleAxis(angle, rotAxis);

			// Apply to local orientation
			const glm::quat currentWorldOrientation = jointTransform.worldOrientation();
			glm::quat newWorldOrientation = worldRotation * currentWorldOrientation;
			newWorldOrientation = glm::normalize(newWorldOrientation);

			// Convert to local space
			glm::quat newLocalOrientation;
			if (jointNode.parent() != InvalidNodeId && sceneGraph.hasNode(jointNode.parent())) {
				const SceneGraphNode &parentNode = sceneGraph.node(jointNode.parent());
				const KeyFrameIndex parentKfIdx = parentNode.keyFrameForFrame(frameIdx);
				const glm::quat parentWorldOrientation = parentNode.transform(parentKfIdx).worldOrientation();
				newLocalOrientation = glm::conjugate(parentWorldOrientation) * newWorldOrientation;
			} else {
				newLocalOrientation = newWorldOrientation;
			}

			// Apply constraints if this joint has an IK constraint
			const IKConstraint *jointConstraint = jointNode.ikConstraint();
			if (jointConstraint != nullptr) {
				newLocalOrientation = clampOrientation(newLocalOrientation, *jointConstraint);
			}

			// Set the new local orientation
			jointTransform.setLocalOrientation(newLocalOrientation);
			jointTransform.update(sceneGraph, jointNode, frameIdx, true);
		}
	}

	return true;
}

} // namespace scenegraph
