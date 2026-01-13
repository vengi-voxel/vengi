/**
 * @file
 */

#include "scenegraph/Physics.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneUtil.h"
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
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));
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
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));
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
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

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
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

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

TEST_F(PhysicsTest, testStairWalking_SingleStep) {
	// Create a scene with stairs (1 voxel high step)
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("stairs");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build first platform at y=0-1
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	// Build second platform (1 voxel higher) at x=16-31, y=0-2
	for (int x = 16; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
			v.setVoxel(x, 2, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));
	ASSERT_EQ(1u, nodes.size());

	// Create a body with height 2 (extents.y = 1.0) which should allow 1 voxel steps
	KinematicBody body;
	body.position = glm::vec3(10.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(2.0f, 0.0f, 0.0f); // Moving toward the step
	body.extents = glm::vec3(0.4f, 1.0f, 0.4f);	 // Height = 2.0, can step 1 voxel
	body.frictionDecay = 0.9f;					 // Reduce friction so body maintains movement

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

	// Apply horizontal velocity toward the step and maintain it (simulating continuous input)
	const float startX = body.position.x;
	const float targetVelocity = 2.0f;

	// Simulate movement - body should climb the step
	for (int i = 0; i < 150; ++i) {
		// Keep applying movement (simulating player holding movement key)
		body.velocity.x = targetVelocity;
		physics.update(deltaTime, nodes, body, gravity);
		// If we've moved past the step
		if (body.position.x > 16.0f) {
			break;
		}
	}

	// Body should have climbed the step and moved forward
	EXPECT_GT(body.position.x, startX) << "Body should have moved forward";
	EXPECT_GT(body.position.x, 16.0f) << "Body should have crossed the step";
	EXPECT_GT(body.position.y, 2.0f) << "Body should be on the higher platform";
	EXPECT_LT(body.position.y, 5.0f) << "Body should not be too high";
}

TEST_F(PhysicsTest, testStairWalking_TwoVoxelStep) {
	// Create a scene with a 2 voxel high step
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("stairs");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build first platform at y=0-1
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	// Build second platform (2 voxels higher) at x=16-31, y=0-3
	for (int x = 16; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
			v.setVoxel(x, 2, z, solidVoxel);
			v.setVoxel(x, 3, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

	// Create a body with height 4 (extents.y = 2.0) which should allow 2 voxel steps
	KinematicBody body;
	body.position = glm::vec3(10.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(2.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 2.0f, 0.4f); // Height = 4.0, can step 2 voxels
	body.frictionDecay = 0.9f;					// Reduce friction

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Let body fall to ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}
	ASSERT_TRUE(body.isGrounded());

	// Apply horizontal velocity and maintain it
	const float startX = body.position.x;
	const float targetVelocity = 2.0f;

	// Simulate movement
	for (int i = 0; i < 150; ++i) {
		body.velocity.x = targetVelocity;
		physics.update(deltaTime, nodes, body, gravity);
		if (body.position.x > 16.0f) {
			break;
		}
	}

	// Body should have climbed the 2-voxel step
	EXPECT_GT(body.position.x, startX) << "Body should have moved forward";
	EXPECT_GT(body.position.x, 16.0f) << "Body should have crossed the step";
	EXPECT_GT(body.position.y, 3.0f) << "Body should be on the higher platform";
}

TEST_F(PhysicsTest, testStairWalking_TooHighStep) {
	// Create a scene with a step that's too high to climb
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("stairs");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build first platform at y=0-1
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	// Build a wall/platform 3 voxels higher at x=16-31, y=0-4
	for (int x = 16; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
			v.setVoxel(x, 2, z, solidVoxel);
			v.setVoxel(x, 3, z, solidVoxel);
			v.setVoxel(x, 4, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

	// Create a body with height 2 (extents.y = 1.0) - can only step 1 voxel
	// But the step is 3 voxels high, so it should NOT be able to climb
	KinematicBody body;
	body.position = glm::vec3(10.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(2.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 1.0f, 0.4f); // Height = 2.0, can only step 1 voxel
	body.frictionDecay = 0.9f;					// Reduce friction

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Let body fall to ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}
	ASSERT_TRUE(body.isGrounded());

	// Apply horizontal velocity and maintain it
	const float targetVelocity = 2.0f;

	// Simulate movement
	for (int i = 0; i < 150; ++i) {
		body.velocity.x = targetVelocity;
		physics.update(deltaTime, nodes, body, gravity);
		if (body.collidedX) {
			break;
		}
	}

	// Body should be blocked - cannot climb a step that's too high
	EXPECT_TRUE(body.collidedX) << "Body should be blocked by the too-high step";
	EXPECT_LT(body.position.x, 16.0f) << "Body should not have crossed the step";
	EXPECT_LT(body.position.y, 4.0f) << "Body should still be on the lower platform";
}

TEST_F(PhysicsTest, testStairWalking_VelocityPreservation) {
	// Test that velocity is preserved during stair climbing (natural movement)
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("stairs");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build first platform
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	// Build second platform (1 voxel higher)
	for (int x = 16; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
			v.setVoxel(x, 2, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

	KinematicBody body;
	body.position = glm::vec3(10.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(2.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 1.0f, 0.4f);
	body.frictionDecay = 0.9f; // Reduce friction

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Get on ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}

	// Set a specific velocity
	const float targetVelocity = 3.0f;

	// Simulate for a few frames as it approaches and climbs the step
	float velocityBeforeStep = 0.0f;
	float velocityAfterStep = 0.0f;
	bool passedStep = false;

	for (int i = 0; i < 150; ++i) {
		// Maintain velocity
		body.velocity.x = targetVelocity;

		if (body.position.x < 15.5f && body.position.x > 14.0f) {
			velocityBeforeStep = body.velocity.x;
		}

		physics.update(deltaTime, nodes, body, gravity);

		if (!passedStep && body.position.x > 16.5f) {
			velocityAfterStep = body.velocity.x;
			passedStep = true;
			break;
		}
	}
	ASSERT_TRUE(passedStep) << "Body should have passed the step";

	// Velocity should be maintained (not reduced to zero)
	// Allow some variance due to friction and numerical precision
	EXPECT_GT(velocityAfterStep, targetVelocity * 0.5f)
		<< "Velocity should be reasonably preserved after stepping (got " << velocityAfterStep << " from "
		<< velocityBeforeStep << ")";
}

TEST_F(PhysicsTest, testStairWalking_MultipleSteps) {
	// Test climbing multiple stairs in succession
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("stairs");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 63, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build a staircase with 4 steps
	for (int step = 0; step < 4; ++step) {
		const int xStart = step * 8;
		const int xEnd = xStart + 8;
		const int yHeight = step + 2; // Each step is 1 voxel higher

		for (int x = xStart; x < xEnd; ++x) {
			for (int z = 0; z <= 31; ++z) {
				for (int y = 0; y < yHeight; ++y) {
					v.setVoxel(x, y, z, solidVoxel);
				}
			}
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

	KinematicBody body;
	body.position = glm::vec3(4.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(3.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 1.0f, 0.4f); // Can step 1 voxel at a time
	body.frictionDecay = 0.9f;					// Reduce friction

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Get on ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}

	const float startX = body.position.x;
	const float targetVelocity = 3.0f;

	// Simulate climbing all steps
	for (int i = 0; i < 400; ++i) {
		// Maintain velocity
		body.velocity.x = targetVelocity;
		physics.update(deltaTime, nodes, body, gravity);
		if (body.position.x > 30.0f) {
			break;
		}
	}

	// Body should have climbed all stairs
	EXPECT_GT(body.position.x, startX + 19.0f) << "Body should have moved significantly forward";
	EXPECT_GT(body.position.y, 3.5f) << "Body should be on a higher platform after multiple steps";
}

TEST_F(PhysicsTest, testStairWalking_NoStepInAir) {
	// Test that stepping up doesn't work in the air
	SceneGraph sceneGraph;
	SceneGraphNode modelNode(SceneGraphNodeType::Model);
	modelNode.setName("platforms");

	voxel::RawVolume v(voxel::Region(0, 0, 0, 31, 31, 31));
	const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Build first platform
	for (int x = 0; x <= 10; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
		}
	}

	// Build second platform with a gap (no ground between)
	for (int x = 20; x <= 31; ++x) {
		for (int z = 0; z <= 31; ++z) {
			v.setVoxel(x, 0, z, solidVoxel);
			v.setVoxel(x, 1, z, solidVoxel);
			v.setVoxel(x, 2, z, solidVoxel);
		}
	}

	modelNode.setVolume(&v, false);
	sceneGraph.emplace(core::move(modelNode));

	CollisionNodes nodes;
	sceneGraph.getCollisionNodes(nodes, 0, scenegraph::toAABB(v.region()));

	KinematicBody body;
	body.position = glm::vec3(9.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(5.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 1.0f, 0.4f);

	Physics physics;
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	// Get on ground
	for (int i = 0; i < 120; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.isGrounded()) {
			break;
		}
	}

	body.velocity.x = 5.0f;

	// Simulate - body should fall into the gap
	for (int i = 0; i < 100; ++i) {
		physics.update(deltaTime, nodes, body, gravity);
		if (body.position.x > 12.0f) {
			break;
		}
	}

	// Body should have fallen (Y position decreased)
	// It should NOT have stepped up to the second platform
	EXPECT_LT(body.position.y, 3.0f) << "Body should have fallen into the gap";
	EXPECT_GT(body.position.x, 10.0f) << "Body should have moved forward off the edge";
}

} // namespace scenegraph
