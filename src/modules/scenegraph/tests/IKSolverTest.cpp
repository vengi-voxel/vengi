/**
 * @file
 */

#include "scenegraph/IKSolver.h"
#include "scenegraph/IKConstraint.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/scalar_constants.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>

namespace scenegraph {

class IKSolverTest : public app::AbstractTest {};

TEST_F(IKSolverTest, testClampOrientationIdentity) {
	IKConstraint constraint;
	const glm::quat identity(1.0f, 0.0f, 0.0f, 0.0f);
	const glm::quat result = IKSolver::clampOrientation(identity, constraint);
	EXPECT_NEAR(result.w, identity.w, 0.001f);
	EXPECT_NEAR(result.x, identity.x, 0.001f);
	EXPECT_NEAR(result.y, identity.y, 0.001f);
	EXPECT_NEAR(result.z, identity.z, 0.001f);
}

TEST_F(IKSolverTest, testClampOrientationRollLimits) {
	IKConstraint constraint;
	constraint.rollMin = -glm::half_pi<float>();
	constraint.rollMax = glm::half_pi<float>();

	// Create a rotation around Y axis (roll) that exceeds the limit
	const glm::quat rotation = glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat result = IKSolver::clampOrientation(rotation, constraint);

	// The result should be clamped to the max roll
	// Extract the twist angle around Y
	const float resultAngle = 2.0f * glm::atan(glm::length(glm::vec3(result.x, result.y, result.z)), result.w);
	EXPECT_LE(resultAngle, glm::half_pi<float>() + 0.1f);
}

TEST_F(IKSolverTest, testSolveNoConstraint) {
	SceneGraph sceneGraph;
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setName("test");
	voxel::Region region(0, 0, 0, 1, 1, 1);
	node.setVolume(new voxel::RawVolume(region), true);
	const int nodeId = sceneGraph.emplace(core::move(node));
	EXPECT_GE(nodeId, 0);
	SceneGraphNode &addedNode = sceneGraph.node(nodeId);
	EXPECT_FALSE(IKSolver::solve(sceneGraph, addedNode, 0));
}

TEST_F(IKSolverTest, testSolveInvalidEffector) {
	SceneGraph sceneGraph;
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setName("test");
	voxel::Region region(0, 0, 0, 1, 1, 1);
	node.setVolume(new voxel::RawVolume(region), true);
	IKConstraint constraint;
	constraint.effectorNodeId = 999; // Non-existent
	node.setIkConstraint(constraint);
	const int nodeId = sceneGraph.emplace(core::move(node));
	EXPECT_GE(nodeId, 0);
	SceneGraphNode &addedNode = sceneGraph.node(nodeId);
	EXPECT_FALSE(IKSolver::solve(sceneGraph, addedNode, 0));
}

TEST_F(IKSolverTest, testSolveSimpleChain) {
	SceneGraph sceneGraph;

	// Create an anchor node (root of the IK chain)
	SceneGraphNode anchor(SceneGraphNodeType::Group);
	anchor.setName("anchor");
	IKConstraint anchorConstraint;
	anchorConstraint.anchor = true;
	anchor.setIkConstraint(anchorConstraint);
	const int anchorId = sceneGraph.emplace(core::move(anchor));
	EXPECT_GE(anchorId, 0);

	// Create a joint node (child of anchor)
	SceneGraphNode joint(SceneGraphNodeType::Group);
	joint.setName("joint");
	const int jointId = sceneGraph.emplace(core::move(joint), anchorId);
	EXPECT_GE(jointId, 0);

	// Create an end-effector node (child of joint)
	SceneGraphNode endEffector(SceneGraphNodeType::Model);
	endEffector.setName("end_effector");
	voxel::Region region(0, 0, 0, 1, 1, 1);
	endEffector.setVolume(new voxel::RawVolume(region), true);
	const int endEffectorId = sceneGraph.emplace(core::move(endEffector), jointId);
	EXPECT_GE(endEffectorId, 0);

	// Create a target node
	SceneGraphNode target(SceneGraphNodeType::Point);
	target.setName("target");
	target.transform(0).setWorldTranslation(glm::vec3(10.0f, 5.0f, 0.0f));
	const int targetId = sceneGraph.emplace(core::move(target));
	EXPECT_GE(targetId, 0);

	// Set up the IK constraint on the end-effector
	SceneGraphNode &endEffectorNode = sceneGraph.node(endEffectorId);
	IKConstraint constraint;
	constraint.effectorNodeId = targetId;
	endEffectorNode.setIkConstraint(constraint);

	// Set up translations to create a chain
	sceneGraph.node(anchorId).transform(0).setLocalTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
	sceneGraph.node(anchorId).transform(0).update(sceneGraph, sceneGraph.node(anchorId), 0, true);
	sceneGraph.node(jointId).transform(0).setLocalTranslation(glm::vec3(5.0f, 0.0f, 0.0f));
	sceneGraph.node(jointId).transform(0).update(sceneGraph, sceneGraph.node(jointId), 0, true);
	sceneGraph.node(endEffectorId).transform(0).setLocalTranslation(glm::vec3(5.0f, 0.0f, 0.0f));
	sceneGraph.node(endEffectorId).transform(0).update(sceneGraph, sceneGraph.node(endEffectorId), 0, true);
	sceneGraph.node(targetId).transform(0).update(sceneGraph, sceneGraph.node(targetId), 0, true);

	// Solve the IK chain
	const bool result = IKSolver::solve(sceneGraph, endEffectorNode, 0);
	EXPECT_TRUE(result);
}

} // namespace scenegraph

