/**
 * @file
 */

#include "scenegraph/Physics.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace scenegraph {

class PhysicsTest : public app::AbstractTest {};

TEST_F(PhysicsTest, testGravityAndGroundCollision) {
	// Create a scene with a ground plane
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("ground");

	// Create a volume with a ground plane at y=0
	voxel::RawVolume v(voxel::Region(0, 0, 0, 15, 15, 15));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build a ground plane (2 layers thick for better collision detection)
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 15; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	// Get collision nodes
	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0);
	ASSERT_EQ(1u, nodes.size()) << "Expected one collision node";

	// Create a body above the ground
	KinematicBody body;
	body.position = glm::vec3(8.0f, 10.0f, 8.0f);
	body.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016; // ~60 FPS

	// Simulate falling for multiple frames
	const glm::vec3 startPosition = body.position;

	// First frame - body should start falling
	physics.update(deltaTime, nodes, body, gravity);
	EXPECT_LT(body.position.y, startPosition.y) << "Body should have moved down due to gravity";
	EXPECT_FALSE(body.isGrounded()) << "Body should not be on ground yet";

	// Simulate for several seconds to ensure body hits ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}

	// Body should be on the ground
	EXPECT_TRUE(body.isGrounded()) << "Body should have hit the ground";
	EXPECT_TRUE(body.collidedY) << "Body should have Y collision";
	EXPECT_GT(body.position.y, 1.5f) << "Body should be above the ground plane (y > 1.5)";
	EXPECT_LT(body.position.y, 4.0f) << "Body should be near the ground (y < 4.0)";
}

TEST_F(PhysicsTest, testWallCollision) {
	// Create a scene with walls
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("walls");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build a ground plane
	for (int x = 0; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
		}
	}

	// Build a wall at x=20
	for (int y = 0; y <= 10; ++y) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(20, y, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0);
	ASSERT_EQ(1u, nodes.size());

	// Create a body moving toward the wall
	KinematicBody body;
	body.position = glm::vec3(15.0f, 5.0f, 15.0f);
	body.velocity = glm::vec3(5.0f, 0.0f, 0.0f); // Moving in +X direction
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Simulate for several frames
	for (int i = 0; i < 60; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.collidedX) {
			break;
		}
	}

	// Body should have hit the wall
	EXPECT_TRUE(body.collidedX) << "Body should have hit the wall";
	EXPECT_LT(body.position.x, 20.0f) << "Body should be stopped before the wall";
}

TEST_F(PhysicsTest, testNoCollisionInEmptySpace) {
	// Create an empty volume
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("empty");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 15, 15, 15));
	// No voxels set - completely empty

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0);

	// Create a body
	KinematicBody body;
	body.position = glm::vec3(8.0f, 8.0f, 8.0f);
	body.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	const glm::vec3 startPosition = body.position;

	// Update once
	physics.update(deltaTime, nodes, body, gravity);

	// Body should fall without collision
	EXPECT_LT(body.position.y, startPosition.y) << "Body should fall due to gravity";
	EXPECT_FALSE(body.isColliding()) << "Body should not collide in empty space";
}

TEST_F(PhysicsTest, testFriction) {
	// Create a scene with a ground plane
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("ground");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build a ground plane
	for (int x = 0; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0);

	// Create a body on the ground with horizontal velocity
	KinematicBody body;
	body.position = glm::vec3(15.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(5.0f, 0.0f, 0.0f); // Moving in +X direction
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);
	body.frictionDecay = 0.1f; // High friction for testing

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Let body fall to ground first
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}

	ASSERT_TRUE(body.isGrounded()) << "Body should be on ground";

	// Now apply horizontal velocity
	body.velocity.x = 5.0f;
	const float startVelocityX = body.velocity.x;

	// Simulate for several frames
	for (int i = 0; i < 30; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
	}

	// Velocity should have decreased due to friction
	EXPECT_LT(glm::abs(body.velocity.x), startVelocityX) << "Friction should have reduced velocity";
}

} // namespace scenegraph
