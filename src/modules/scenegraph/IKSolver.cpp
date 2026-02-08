/**
 * @file
 */

#include "IKSolver.h"
#include "SceneGraph.h"
#include "SceneGraphNode.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/norm.hpp>

namespace scenegraph {

glm::quat IKSolver::clampOrientation(const glm::quat &localOrientation, const IKConstraint &constraint) {
	// Decompose into swing-twist around Y axis (the typical joint axis)
	// twist = rotation around the Y axis, swing = rotation that brings Y to the desired direction
	const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
	const glm::vec3 rotatedAxis = localOrientation * yAxis;

	// Extract twist component
	const float dot = glm::dot(yAxis, rotatedAxis);
	glm::quat swing;
	glm::quat twist;

	if (glm::abs(dot) > 0.9999f) {
		// Aligned or anti-aligned - no swing needed
		twist = localOrientation;
		swing = glm::quat_identity<float, glm::defaultp>();
	} else {
		const glm::vec3 projection = yAxis * glm::dot(glm::vec3(localOrientation.x, localOrientation.y, localOrientation.z), yAxis);
		twist = glm::normalize(glm::quat(localOrientation.w, projection));
		swing = localOrientation * glm::conjugate(twist);
	}

	// Clamp the twist (roll) angle
	float twistAngle = 2.0f * glm::atan(glm::length(glm::vec3(twist.x, twist.y, twist.z)), twist.w);
	if (twistAngle > glm::pi<float>()) {
		twistAngle -= 2.0f * glm::pi<float>();
	}

	twistAngle = glm::clamp(twistAngle, constraint.rollMin, constraint.rollMax);
	twist = glm::angleAxis(twistAngle, yAxis);

	// Clamp swing using the swing limits if any are defined
	if (!constraint.swingLimits.empty()) {
		const glm::vec3 swingAxis = swing * yAxis;
		// Convert to spherical coordinates to check against the constraint cones
		const float swingAngle = glm::acos(glm::clamp(glm::dot(swingAxis, yAxis), -1.0f, 1.0f));
		if (swingAngle > 0.001f) {
			// Find the maximum allowed swing angle from the constraint cones
			float maxSwingAngle = glm::pi<float>();
			for (const auto &limit : constraint.swingLimits) {
				maxSwingAngle = glm::min(maxSwingAngle, limit.radius);
			}
			if (swingAngle > maxSwingAngle) {
				// Clamp the swing angle
				const glm::vec3 swingRotAxis = glm::cross(yAxis, swingAxis);
				if (glm::length2(swingRotAxis) > 0.0001f) {
					swing = glm::angleAxis(maxSwingAngle, glm::normalize(swingRotAxis));
				}
			}
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

