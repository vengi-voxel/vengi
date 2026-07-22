/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/Physics.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class PhysicsBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraph _sceneGraph;
	scenegraph::Physics _physics;
	scenegraph::CollisionNodes _nodes;
	voxel::RawVolume *_volume = nullptr;

	void createGroundPlane() {
		_volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 31, 31, 31));
		const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

		for (int x = 0; x <= 31; ++x) {
			for (int z = 0; z <= 31; ++z) {
				_volume->setVoxel(x, 0, z, solidVoxel);
				_volume->setVoxel(x, 1, z, solidVoxel);
			}
		}

		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.setName("ground");
		modelNode.setUnownedVolume(_volume);
		_sceneGraph.emplace(core::move(modelNode));
		_sceneGraph.getCollisionNodes(_nodes, 0, scenegraph::toAABB(_volume->region()));
	}

	void createComplexScene() {
		_volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 31, 31, 31));
		const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

		for (int x = 0; x <= 31; ++x) {
			for (int z = 0; z <= 31; ++z) {
				_volume->setVoxel(x, 0, z, solidVoxel);
			}
		}

		for (int y = 1; y <= 10; ++y) {
			for (int x = 0; x <= 31; ++x) {
				_volume->setVoxel(x, y, 0, solidVoxel);
				_volume->setVoxel(x, y, 31, solidVoxel);
			}
			for (int z = 0; z <= 31; ++z) {
				_volume->setVoxel(0, y, z, solidVoxel);
				_volume->setVoxel(31, y, z, solidVoxel);
			}
		}

		for (int step = 0; step < 10; ++step) {
			for (int x = 10; x <= 12; ++x) {
				for (int z = 10 + step; z <= 10 + step; ++z) {
					for (int y = 0; y <= step; ++y) {
						_volume->setVoxel(x, y, z, solidVoxel);
					}
				}
			}
		}

		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.setName("complex");
		modelNode.setUnownedVolume(_volume);
		_sceneGraph.emplace(core::move(modelNode));
		_sceneGraph.getCollisionNodes(_nodes, 0, scenegraph::toAABB(_volume->region()));
	}

	void createPillarField() {
		_volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 63, 31, 63));
		const voxel::Voxel solidVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

		for (int x = 0; x <= 63; ++x) {
			for (int z = 0; z <= 63; ++z) {
				_volume->setVoxel(x, 0, z, solidVoxel);
			}
		}
		// Dense pillar field - stresses AABB voxel sampling while moving
		for (int x = 2; x <= 60; x += 3) {
			for (int z = 2; z <= 60; z += 3) {
				for (int y = 1; y <= 8; ++y) {
					_volume->setVoxel(x, y, z, solidVoxel);
				}
			}
		}

		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.setName("pillars");
		modelNode.setUnownedVolume(_volume);
		_sceneGraph.emplace(core::move(modelNode));
		_sceneGraph.getCollisionNodes(_nodes, 0, scenegraph::toAABB(_volume->region()));
	}

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		_sceneGraph = scenegraph::SceneGraph();
	}

	void TearDown(::benchmark::State &state) override {
		delete _volume;
		_volume = nullptr;
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(PhysicsBenchmark, UpdateGravityOnly)(benchmark::State &state) {
	createGroundPlane();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(15.0f, 10.0f, 15.0f);
	body.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);

	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.position);
	}
}

BENCHMARK_DEFINE_F(PhysicsBenchmark, UpdateWithHorizontalMovement)(benchmark::State &state) {
	createGroundPlane();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(15.0f, 3.0f, 15.0f);
	body.velocity = glm::vec3(5.0f, 0.0f, 3.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);

	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.position);
		benchmark::DoNotOptimize(body.velocity);
	}
}

BENCHMARK_DEFINE_F(PhysicsBenchmark, UpdateWithStairClimbing)(benchmark::State &state) {
	createComplexScene();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(11.0f, 3.0f, 8.0f);
	body.velocity = glm::vec3(0.0f, 0.0f, 2.0f);
	body.extents = glm::vec3(0.2f, 1.0f, 0.2f);

	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		if (body.position.z > 20.0f) {
			body.position.z = 8.0f;
		}
		body.velocity.z = 2.0f;
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.position);
	}
}

BENCHMARK_DEFINE_F(PhysicsBenchmark, UpdateWithFriction)(benchmark::State &state) {
	createGroundPlane();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(15.0f, 3.0f, 15.0f);
	body.velocity = glm::vec3(5.0f, 0.0f, 0.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);
	body.frictionDecay = 0.1f;

	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.velocity);
	}
}

// Sustained falling: reset each iteration so we always measure mid-air + impact work
BENCHMARK_DEFINE_F(PhysicsBenchmark, RepeatedFall)(benchmark::State &state) {
	createGroundPlane();

	scenegraph::KinematicBody body;
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		body.position = glm::vec3(15.0f, 12.0f, 15.0f);
		body.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		body.collidedX = body.collidedY = body.collidedZ = false;
		for (int i = 0; i < 45; ++i) {
			_physics.update(deltaTime, _nodes, body, gravity);
		}
		benchmark::DoNotOptimize(body.position);
	}
}

// Continuous slide along a wall while grounded - keeps X binary search hot
BENCHMARK_DEFINE_F(PhysicsBenchmark, WallSlide)(benchmark::State &state) {
	createComplexScene();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(2.0f, 3.0f, 15.0f);
	body.velocity = glm::vec3(-5.0f, 0.0f, 4.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		if (body.position.z > 28.0f || body.position.z < 2.0f) {
			body.position.z = 15.0f;
			body.velocity.z = 4.0f;
		}
		body.velocity.x = -5.0f;
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.position);
	}
}

// Move through a dense pillar field - stresses per-voxel sampling in checkCollision
BENCHMARK_DEFINE_F(PhysicsBenchmark, PillarFieldTraverse)(benchmark::State &state) {
	createPillarField();

	scenegraph::KinematicBody body;
	body.position = glm::vec3(4.0f, 3.0f, 4.0f);
	body.velocity = glm::vec3(6.0f, 0.0f, 5.0f);
	body.extents = glm::vec3(0.4f, 0.8f, 0.4f);
	const float gravity = 9.81f;
	const double deltaTime = 0.016;

	for (auto _ : state) {
		if (body.position.x > 58.0f || body.position.z > 58.0f) {
			body.position = glm::vec3(4.0f, 3.0f, 4.0f);
			body.velocity = glm::vec3(6.0f, 0.0f, 5.0f);
			body.collidedX = body.collidedY = body.collidedZ = false;
		}
		_physics.update(deltaTime, _nodes, body, gravity);
		benchmark::DoNotOptimize(body.position);
	}
}

BENCHMARK_REGISTER_F(PhysicsBenchmark, UpdateGravityOnly);
BENCHMARK_REGISTER_F(PhysicsBenchmark, UpdateWithHorizontalMovement);
BENCHMARK_REGISTER_F(PhysicsBenchmark, UpdateWithStairClimbing);
BENCHMARK_REGISTER_F(PhysicsBenchmark, UpdateWithFriction);
BENCHMARK_REGISTER_F(PhysicsBenchmark, RepeatedFall);
BENCHMARK_REGISTER_F(PhysicsBenchmark, WallSlide);
BENCHMARK_REGISTER_F(PhysicsBenchmark, PillarFieldTraverse);
