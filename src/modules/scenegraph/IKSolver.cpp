/**
 * @file
 */

#include "IKSolver.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "math/Quatf.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"
#include "math/Scalar.h"


namespace scenegraph {

/**
 * @brief Convert a swing limit's polar center (yaw, pitch) to a 3D unit direction vector.
 * Applies rotateX(yaw) then rotateZ(pitch) to the Y-up vector.
 */
static math::Vec3f coneDirection(const math::Vec2f &center) {
	const float cx = math::cos(center.x), sx = math::sin(center.x);
	const float cz = math::cos(center.y), sz = math::sin(center.y);
	return math::Vec3f(-sz * cx, cz * cx, sx);
}

/**
 * @brief Rotate unit vector @p v toward @p target by @p angle radians.
 */
static math::Vec3f rotateToward(const math::Vec3f &v, const math::Vec3f &target, float angle) {
	const math::Vec3f axis = math::cross(v, target);
	if (math::length2(axis) < 0.0001f) {
		return v;
	}
	return math::normalize(math::angleAxis(angle, math::normalize(axis)) * v);
}

/**
 * @brief Build a swing quaternion that rotates Y-axis to the given direction.
 */
static math::Quatf swingFromDirection(const math::Vec3f &dir) {
	const math::Vec3f yAxis(0.0f, 1.0f, 0.0f);
	const math::Vec3f axis = math::cross(yAxis, dir);
	if (math::length2(axis) < 0.0001f) {
		return math::Quatf::identity();
	}
	const float angle = math::acos(math::clamp(math::dot(yAxis, dir), -1.0f, 1.0f));
	return math::angleAxis(angle, math::normalize(axis));
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
static bool checkConePairPath(const math::Vec3f &coneA, float radiusA, const math::Vec3f &coneB, float radiusB,
							  const math::Vec3f &input, math::Vec3f &result) {
	const float tRadius = (math::pi - (radiusA + radiusB)) / 2.0f;
	if (tRadius <= 0.0f) {
		return false;
	}

	const math::Vec3f arcNormal = math::cross(coneA, coneB);
	if (math::length2(arcNormal) < 0.0001f) {
		return false;
	}
	const math::Vec3f normArc = math::normalize(arcNormal);

	// Compute two tangent circle centers (one on each side of the great arc)
	const math::Vec3f tan1 = math::normalize(math::angleAxis(radiusA + tRadius, normArc) * coneA);
	const math::Vec3f tan2 = math::normalize(math::angleAxis(-(radiusA + tRadius), normArc) * coneA);

	// Determine which side of the great arc the input is on
	const float side = math::dot(input, math::cross(coneA, coneB));
	const math::Vec3f &tanCenter = (side < 0.0f) ? tan1 : tan2;

	// Check if input is in the triangle region bounded by coneA, tanCenter, coneB
	// using cross-product half-plane tests
	const math::Vec3f edge1 = (side < 0.0f) ? math::cross(coneA, tanCenter) : math::cross(tanCenter, coneA);
	const math::Vec3f edge2 = (side < 0.0f) ? math::cross(tanCenter, coneB) : math::cross(coneB, tanCenter);

	if (math::dot(input, edge1) <= 0.0f || math::dot(input, edge2) <= 0.0f) {
		return false;
	}

	// In the path region. Check if inside the tangent circle (= out of bounds, needs clamping)
	if (math::dot(input, tanCenter) > math::cos(tRadius)) {
		result = math::normalize(rotateToward(tanCenter, input, tRadius));
	} else {
		result = input;
	}
	return true;
}

math::Quatf IKSolver::clampOrientation(const math::Quatf &localOrientation, const IKConstraint &constraint) {
	const math::Vec3f yAxis(0.0f, 1.0f, 0.0f);
	const math::Vec3f rotatedAxis = localOrientation * yAxis;

	math::Quatf swing;
	math::Quatf twist;

	if (math::abs(math::dot(yAxis, rotatedAxis)) > 0.9999f) {
		twist = localOrientation;
		swing = math::Quatf::identity();
	} else {
		const math::Vec3f projection =
			yAxis * math::dot(math::Vec3f(localOrientation.x, localOrientation.y, localOrientation.z), yAxis);
		twist = math::normalize(math::Quatf(localOrientation.w, projection));
		swing = localOrientation * math::conjugate(twist);
	}

	// Clamp twist to [rollMin, rollMax] using [0, 2*PI] convention
	float twistAngle = 2.0f * math::atan(math::length(math::Vec3f(twist.x, twist.y, twist.z)), twist.w);
	if (twistAngle < 0.0f) {
		twistAngle += math::twoPi;
	}
	if (twistAngle < constraint.rollMin || twistAngle > constraint.rollMax) {
		const float distToMin = math::min(math::abs(twistAngle - constraint.rollMin),
										 math::twoPi - math::abs(twistAngle - constraint.rollMin));
		const float distToMax = math::min(math::abs(twistAngle - constraint.rollMax),
										 math::twoPi - math::abs(twistAngle - constraint.rollMax));
		twistAngle = (distToMin <= distToMax) ? constraint.rollMin : constraint.rollMax;
	}
	twist = math::angleAxis(twistAngle, yAxis);

	// Clamp swing: per-cone containment + inter-cone path interpolation
	const int numCones = (int)constraint.swingLimits.size();
	if (numCones > 0) {
		const math::Vec3f swingDir = math::normalize(swing * yAxis);

		// Step 1: Check if inside any individual cone
		bool inBounds = false;
		for (int i = 0; i < numCones; ++i) {
			if (math::dot(swingDir, coneDirection(constraint.swingLimits[i].center)) >=
				math::cos(constraint.swingLimits[i].radius)) {
				inBounds = true;
				break;
			}
		}

		// Step 2: Check inter-cone path regions between consecutive cones
		if (!inBounds && numCones > 1) {
			for (int i = 0; i < numCones - 1; ++i) {
				const auto &limA = constraint.swingLimits[i];
				const auto &limB = constraint.swingLimits[i + 1];
				math::Vec3f pathResult;
				if (checkConePairPath(coneDirection(limA.center), limA.radius, coneDirection(limB.center), limB.radius,
									  swingDir, pathResult)) {
					inBounds = true;
					// pathResult == swingDir means already in bounds, otherwise clamped
					if (math::distance(pathResult, swingDir) > 0.0001f) {
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
				const float d = math::dot(swingDir, coneDirection(constraint.swingLimits[i].center));
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

	const core::UUID &effectorUUID = constraint->effectorUUID;
	if (!effectorUUID.isValid()) {
		return false;
	}

	const SceneGraphNode *effectorPtr = sceneGraph.findNodeByUUID(effectorUUID);
	if (effectorPtr == nullptr) {
		return false;
	}

	// Build the chain from the node up to the anchor
	core::DynamicArray<int> chain;
	chain.push_back(node.id());

	const SceneGraphNode *current = sceneGraph.parentNode(node);
	while (current != nullptr) {
		chain.push_back(current->id());
		if (current->isIKAnchor() || current->isRootNode()) {
			break;
		}
		current = sceneGraph.parentNode(*current);
	}

	if (chain.size() < 2) {
		return false;
	}

	// The target position is the world position of the effector node
	const SceneGraphNode &effectorNode = *effectorPtr;
	const KeyFrameIndex effectorKeyFrameIdx = effectorNode.keyFrameForFrame(frameIdx);
	const math::Vec3f targetPos = effectorNode.transform(effectorKeyFrameIdx).worldTranslation();

	// Get the end-effector position (the node itself)
	for (int iter = 0; iter < MaxIterations; ++iter) {
		// Check if we've reached the target
		const KeyFrameIndex nodeKeyFrameIdx = node.keyFrameForFrame(frameIdx);
		const math::Vec3f endEffectorPos = node.transform(nodeKeyFrameIdx).worldTranslation();
		const float dist = math::distance(endEffectorPos, targetPos);
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
			const math::Vec3f jointPos = jointTransform.worldTranslation();

			// Recompute end effector position after previous joint adjustments
			const KeyFrameIndex currentNodeKeyFrameIdx = node.keyFrameForFrame(frameIdx);
			const math::Vec3f currentEndPos = node.transform(currentNodeKeyFrameIdx).worldTranslation();

			// Vectors from joint to end-effector and target
			math::Vec3f toEnd = currentEndPos - jointPos;
			math::Vec3f toTarget = targetPos - jointPos;

			const float toEndLen = math::length(toEnd);
			const float toTargetLen = math::length(toTarget);

			if (toEndLen < 0.0001f || toTargetLen < 0.0001f) {
				continue;
			}

			toEnd /= toEndLen;
			toTarget /= toTargetLen;

			// Calculate the rotation needed
			const float dotProduct = math::clamp(math::dot(toEnd, toTarget), -1.0f, 1.0f);
			if (dotProduct > 0.9999f) {
				continue; // Already aligned
			}

			const float angle = math::acos(dotProduct);
			math::Vec3f rotAxis = math::cross(toEnd, toTarget);

			if (math::length2(rotAxis) < 0.0001f) {
				continue;
			}
			rotAxis = math::normalize(rotAxis);

			// Create the rotation in world space
			const math::Quatf worldRotation = math::angleAxis(angle, rotAxis);

			// Apply to local orientation
			const math::Quatf currentWorldOrientation = jointTransform.worldOrientation();
			math::Quatf newWorldOrientation = worldRotation * currentWorldOrientation;
			newWorldOrientation = math::normalize(newWorldOrientation);

			// Convert to local space
			math::Quatf newLocalOrientation;
			if (const SceneGraphNode *parentNode = sceneGraph.parentNode(jointNode)) {
				const KeyFrameIndex parentKfIdx = parentNode->keyFrameForFrame(frameIdx);
				const math::Quatf parentWorldOrientation = parentNode->transform(parentKfIdx).worldOrientation();
				newLocalOrientation = math::conjugate(parentWorldOrientation) * newWorldOrientation;
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
