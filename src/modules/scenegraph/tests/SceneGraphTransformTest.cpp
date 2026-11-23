/**
 * @file
 */

#include "scenegraph/SceneGraphTransform.h"
#include "app/tests/AbstractTest.h"
#include "color/Color.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"
#include "math/Scalar.h"
#include "math/Print.h"

namespace scenegraph {

class SceneGraphTransformTest : public app::AbstractTest {};

TEST_F(SceneGraphTransformTest, testWorldToLocalTranslation) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent node at position (10, 20, 30)
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(10.0f, 20.0f, 30.0f));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child node
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	// Update transforms to calculate world matrices
	sceneGraph.updateTransforms();

	// Get the child node and set its world position to (15, 25, 35)
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	childTransform.setWorldTranslation(math::Vec3f(15.0f, 25.0f, 35.0f));
	childTransform.update(sceneGraph, child, 0, false);

	// The local translation should be the difference: (5, 5, 5)
	const math::Vec3f expectedLocal(5.0f, 5.0f, 5.0f);
	const math::Vec3f &actualLocal = childTransform.localTranslation();
	EXPECT_VEC_NEAR(actualLocal, expectedLocal, 0.0001f)
		<< "Local: " << math::to_string(actualLocal) << ", Expected: " << math::to_string(expectedLocal);
}

TEST_F(SceneGraphTransformTest, testWorldToLocalWithRotation) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent node with 90-degree rotation around Y axis
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(10.0f, 0.0f, 0.0f));
	parentTransform.setLocalOrientation(math::angleAxis(math::halfPi, math::Vec3f(0.0f, 1.0f, 0.0f)));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child node
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Set child world position to (10, 0, 10) - which is 10 units forward from parent
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	childTransform.setWorldTranslation(math::Vec3f(10.0f, 0.0f, 10.0f));
	childTransform.update(sceneGraph, child, 0, false);

	// Due to parent's 90-degree Y rotation, local (0, 0, 10) becomes world (10, 0, 10) relative to parent at (10, 0, 0)
	// So the local position should be approximately (-10, 0, 0) after inverse rotation
	const math::Vec3f &actualLocal = childTransform.localTranslation();
	EXPECT_NEAR(actualLocal.x, -10.0f, 0.01f) << "Local translation X: " << math::to_string(actualLocal);
	EXPECT_NEAR(actualLocal.y, 0.0f, 0.01f) << "Local translation Y: " << math::to_string(actualLocal);
	EXPECT_NEAR(actualLocal.z, 0.0f, 0.01f) << "Local translation Z: " << math::to_string(actualLocal);
}

TEST_F(SceneGraphTransformTest, testWorldToLocalWithScale) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent node with scale (2, 2, 2)
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(10.0f, 0.0f, 0.0f));
	parentTransform.setLocalScale(math::Vec3f(2.0f, 2.0f, 2.0f));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child node
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Set child world position to (20, 0, 0)
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	childTransform.setWorldTranslation(math::Vec3f(20.0f, 0.0f, 0.0f));
	childTransform.update(sceneGraph, child, 0, false);

	// Local translation should account for parent's scale: (20 - 10) / 2 = (5, 0, 0)
	const math::Vec3f expectedLocal(5.0f, 0.0f, 0.0f);
	const math::Vec3f &actualLocal = childTransform.localTranslation();
	EXPECT_VEC_NEAR(actualLocal, expectedLocal, 0.01f)
		<< "Local: " << math::to_string(actualLocal) << ", Expected: " << math::to_string(expectedLocal);
}

TEST_F(SceneGraphTransformTest, testWorldToLocalScale) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent node with scale (2, 2, 2)
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalScale(math::Vec3f(2.0f, 2.0f, 2.0f));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child node
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Set child world scale to (4, 4, 4)
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	childTransform.setWorldScale(math::Vec3f(4.0f, 4.0f, 4.0f));
	childTransform.update(sceneGraph, child, 0, false);

	// Local scale should be: worldScale / parentWorldScale = (4, 4, 4) / (2, 2, 2) = (2, 2, 2)
	const math::Vec3f expectedLocal(2.0f, 2.0f, 2.0f);
	const math::Vec3f &actualLocal = childTransform.localScale();
	EXPECT_VEC_NEAR(actualLocal, expectedLocal, 0.0001f)
		<< "Local: " << math::to_string(actualLocal) << ", Expected: " << math::to_string(expectedLocal);
}

TEST_F(SceneGraphTransformTest, testWorldToLocalOrientation) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent node with 45-degree rotation around Y axis
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	const math::Quatf parentRot = math::angleAxis(math::quarterPi, math::Vec3f(0.0f, 1.0f, 0.0f));
	parentTransform.setLocalOrientation(parentRot);
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child node
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Set child world orientation to 90-degree rotation around Y axis
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	const math::Quatf worldRot = math::angleAxis(math::halfPi, math::Vec3f(0.0f, 1.0f, 0.0f));
	childTransform.setWorldOrientation(worldRot);
	childTransform.update(sceneGraph, child, 0, false);

	// Local orientation should be: worldOrientation * inverse(parentWorldOrientation)
	// 90 degree * inverse(45 degree) = 45 degree around Y
	const math::Quatf expectedLocal = math::angleAxis(math::quarterPi, math::Vec3f(0.0f, 1.0f, 0.0f));
	const math::Quatf &actualLocal = childTransform.localOrientation();

	// Compare quaternions - they can be negated and still represent the same rotation
	bool equal = math::all(math::epsilonEqual(actualLocal, expectedLocal, 0.001f)) ||
				 math::all(math::epsilonEqual(actualLocal, -expectedLocal, 0.001f));
	EXPECT_TRUE(equal) << "Local: " << math::to_string(actualLocal) << ", Expected: " << math::to_string(expectedLocal);
}

TEST_F(SceneGraphTransformTest, testMultiLevelHierarchy) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create grandparent at (10, 0, 0)
	SceneGraphNode grandparentNode(SceneGraphNodeType::Model);
	grandparentNode.setName("grandparent");
	grandparentNode.setUnownedVolume(&v);
	SceneGraphTransform grandparentTransform;
	grandparentTransform.setLocalTranslation(math::Vec3f(10.0f, 0.0f, 0.0f));
	grandparentNode.setTransform(0, grandparentTransform);
	int grandparentId = sceneGraph.emplace(core::move(grandparentNode), 0);
	ASSERT_GT(grandparentId, 0);

	// Create parent at local (5, 0, 0), world (15, 0, 0)
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(5.0f, 0.0f, 0.0f));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), grandparentId);
	ASSERT_GT(parentId, 0);

	// Create child at local (3, 0, 0), world (18, 0, 0)
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	SceneGraphTransform childTransform;
	childTransform.setLocalTranslation(math::Vec3f(3.0f, 0.0f, 0.0f));
	childNode.setTransform(0, childTransform);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	// Update all transforms
	sceneGraph.updateTransforms();

	// Verify world positions
	const SceneGraphNode &parent = sceneGraph.node(parentId);
	const math::Vec3f &parentWorld = parent.transform(0).worldTranslation();
	EXPECT_VEC_NEAR(parentWorld, math::Vec3f(15.0f, 0.0f, 0.0f), 0.0001f)
		<< "Parent world: " << math::to_string(parentWorld);

	const SceneGraphNode &child = sceneGraph.node(childId);
	const math::Vec3f &childWorld = child.transform(0).worldTranslation();
	EXPECT_VEC_NEAR(childWorld, math::Vec3f(18.0f, 0.0f, 0.0f), 0.0001f) << "Child world: " << math::to_string(childWorld);
}

TEST_F(SceneGraphTransformTest, testChangeParentKeepWorldTransform) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent1 at (10, 0, 0) with scale (2, 2, 2)
	SceneGraphNode parent1Node(SceneGraphNodeType::Model);
	parent1Node.setName("parent1");
	parent1Node.setUnownedVolume(&v);
	SceneGraphTransform parent1Transform;
	parent1Transform.setLocalTranslation(math::Vec3f(10.0f, 0.0f, 0.0f));
	parent1Transform.setLocalScale(math::Vec3f(2.0f, 2.0f, 2.0f));
	parent1Node.setTransform(0, parent1Transform);
	int parent1Id = sceneGraph.emplace(core::move(parent1Node), 0);
	ASSERT_GT(parent1Id, 0);

	// Create parent2 at (5, 5, 5)
	SceneGraphNode parent2Node(SceneGraphNodeType::Model);
	parent2Node.setName("parent2");
	parent2Node.setUnownedVolume(&v);
	SceneGraphTransform parent2Transform;
	parent2Transform.setLocalTranslation(math::Vec3f(5.0f, 5.0f, 5.0f));
	parent2Node.setTransform(0, parent2Transform);
	int parent2Id = sceneGraph.emplace(core::move(parent2Node), 0);
	ASSERT_GT(parent2Id, 0);

	// Create child under parent1 at local (5, 0, 0), which gives world position (20, 0, 0)
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	SceneGraphTransform childTransform;
	childTransform.setLocalTranslation(math::Vec3f(5.0f, 0.0f, 0.0f));
	childNode.setTransform(0, childTransform);
	int childId = sceneGraph.emplace(core::move(childNode), parent1Id);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Store the child's world transform before reparenting
	const SceneGraphNode &childBefore = sceneGraph.node(childId);
	const math::Vec3f worldPosBefore = childBefore.transform(0).worldTranslation();
	const math::Quatf worldRotBefore = childBefore.transform(0).worldOrientation();
	const math::Vec3f worldScaleBefore = childBefore.transform(0).worldScale();

	EXPECT_VEC_NEAR(worldPosBefore, math::Vec3f(20.0f, 0.0f, 0.0f), 0.0001f)
		<< "World position before: " << math::to_string(worldPosBefore);

	// Change parent from parent1 to parent2, keeping world transform
	ASSERT_TRUE(sceneGraph.changeParent(childId, parent2Id, NodeMoveFlag::KeepWorldTransform));

	// Verify the child's world transform remains the same
	const SceneGraphNode &childAfter = sceneGraph.node(childId);
	const math::Vec3f worldPosAfter = childAfter.transform(0).worldTranslation();
	const math::Quatf worldRotAfter = childAfter.transform(0).worldOrientation();
	const math::Vec3f worldScaleAfter = childAfter.transform(0).worldScale();

	EXPECT_VEC_NEAR(worldPosAfter, worldPosBefore, 0.01f)
		<< "World position should remain unchanged. Before: " << math::to_string(worldPosBefore)
		<< ", After: " << math::to_string(worldPosAfter);

	bool orientationEqual = math::all(math::epsilonEqual(worldRotAfter, worldRotBefore, 0.001f)) ||
							math::all(math::epsilonEqual(worldRotAfter, -worldRotBefore, 0.001f));
	EXPECT_TRUE(orientationEqual) << "World orientation should remain unchanged. Before: "
								  << math::to_string(worldRotBefore) << ", After: " << math::to_string(worldRotAfter);

	EXPECT_VEC_NEAR(worldScaleAfter, worldScaleBefore, 0.01f)
		<< "World scale should remain unchanged. Before: " << math::to_string(worldScaleBefore)
		<< ", After: " << math::to_string(worldScaleAfter);

	// But the local transform should have changed
	const math::Vec3f &localPosAfter = childAfter.transform(0).localTranslation();
	// New local position should be (20 - 5, 0 - 5, 0 - 5) = (15, -5, -5)
	EXPECT_VEC_NEAR(localPosAfter, math::Vec3f(15.0f, -5.0f, -5.0f), 0.01f)
		<< "Local position after reparenting: " << math::to_string(localPosAfter);
}

TEST_F(SceneGraphTransformTest, testLocalToWorldUpdate) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	// Create parent
	SceneGraphNode parentNode(SceneGraphNodeType::Model);
	parentNode.setName("parent");
	parentNode.setUnownedVolume(&v);
	SceneGraphTransform parentTransform;
	parentTransform.setLocalTranslation(math::Vec3f(10.0f, 0.0f, 0.0f));
	parentNode.setTransform(0, parentTransform);
	int parentId = sceneGraph.emplace(core::move(parentNode), 0);
	ASSERT_GT(parentId, 0);

	// Create child
	SceneGraphNode childNode(SceneGraphNodeType::Model);
	childNode.setName("child");
	childNode.setUnownedVolume(&v);
	int childId = sceneGraph.emplace(core::move(childNode), parentId);
	ASSERT_GT(childId, 0);

	sceneGraph.updateTransforms();

	// Modify the child's local translation
	SceneGraphNode &child = sceneGraph.node(childId);
	SceneGraphTransform &childTransform = child.transform(0);
	childTransform.setLocalTranslation(math::Vec3f(5.0f, 3.0f, 2.0f));
	childTransform.update(sceneGraph, child, 0, false);

	// World position should be parent + local = (10, 0, 0) + (5, 3, 2) = (15, 3, 2)
	const math::Vec3f &worldPos = childTransform.worldTranslation();
	EXPECT_VEC_NEAR(worldPos, math::Vec3f(15.0f, 3.0f, 2.0f), 0.0001f) << "World position: " << math::to_string(worldPos);
}

TEST_F(SceneGraphTransformTest, testMatrixDecompositionConsistency) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setName("node");
	node.setUnownedVolume(&v);
	SceneGraphTransform transform;

	// Set up a complex transform
	const math::Vec3f translation(5.0f, 10.0f, 15.0f);
	const math::Quatf orientation = math::angleAxis(math::quarterPi, math::normalize(math::Vec3f(1.0f, 1.0f, 0.0f)));
	const math::Vec3f scale(1.5f, 2.0f, 1.0f);

	transform.setLocalTranslation(translation);
	transform.setLocalOrientation(orientation);
	transform.setLocalScale(scale);
	node.setTransform(0, transform);

	int nodeId = sceneGraph.emplace(core::move(node), 0);
	ASSERT_GT(nodeId, 0);

	sceneGraph.updateTransforms();

	// Get the transform and verify the matrix represents the same TRS
	const SceneGraphNode &resultNode = sceneGraph.node(nodeId);
	const SceneGraphTransform &resultTransform = resultNode.transform(0);
	const math::Mat4f &worldMatrix = resultTransform.worldMatrix();

	// Manually construct the expected matrix
	const math::Mat4f expectedMatrix = math::translate(translation) * math::mat4_cast(orientation) * math::scale(scale);

	// Compare matrices
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			EXPECT_NEAR(worldMatrix[i][j], expectedMatrix[i][j], 0.0001f)
				<< "Matrix element [" << i << "][" << j << "] differs";
		}
	}
}

TEST_F(SceneGraphTransformTest, testDirtyFlagManagement) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));

	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setName("node");
	node.setUnownedVolume(&v);
	SceneGraphTransform transform;
	node.setTransform(0, transform);

	int nodeId = sceneGraph.emplace(core::move(node), 0);
	ASSERT_GT(nodeId, 0);

	sceneGraph.updateTransforms();

	SceneGraphNode &resultNode = sceneGraph.node(nodeId);
	SceneGraphTransform &resultTransform = resultNode.transform(0);

	// After update, transform should not be dirty
	EXPECT_FALSE(resultTransform.dirty()) << "Transform should not be dirty after update";

	// Modify local translation
	resultTransform.setLocalTranslation(math::Vec3f(1.0f, 2.0f, 3.0f));
	EXPECT_TRUE(resultTransform.dirty()) << "Transform should be dirty after modification";

	// Update and verify it's clean again
	resultTransform.update(sceneGraph, resultNode, 0, false);
	EXPECT_FALSE(resultTransform.dirty()) << "Transform should not be dirty after update";
}

} // namespace scenegraph
