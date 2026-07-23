/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/CollisionNode.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

class SceneGraphBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraph _sceneGraph;

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		_sceneGraph = scenegraph::SceneGraph();
	}
};

BENCHMARK_DEFINE_F(SceneGraphBenchmark, Init)(benchmark::State &state) {
	for (auto _ : state) {
		scenegraph::SceneGraph sceneGraph;
	}
}

BENCHMARK_DEFINE_F(SceneGraphBenchmark, SizeModel)(benchmark::State &state) {
	for (auto _ : state) {
		_sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
	}
}

BENCHMARK_DEFINE_F(SceneGraphBenchmark, SceneGraphNode)(benchmark::State &state) {
	for (auto _ : state) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	}
}

BENCHMARK_DEFINE_F(SceneGraphBenchmark, GetCollisionNodesMany)(benchmark::State &state) {
	scenegraph::SceneGraph sceneGraph;
	const int nodeCount = 64;
	for (int i = 0; i < nodeCount; ++i) {
		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.createVolume(voxel::Region(0, 0, 0, 3, 3, 3));
		modelNode.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3((float)(i % 8) * 10.0f, 0.0f, (float)(i / 8) * 10.0f));
		modelNode.setTransform(0, transform);
		sceneGraph.emplace(core::move(modelNode));
	}
	sceneGraph.updateTransforms();

	const math::AABB<float> queryAABB(glm::vec3(-5.0f, -5.0f, -5.0f), glm::vec3(85.0f, 10.0f, 85.0f));
	for (auto _ : state) {
		scenegraph::CollisionNodes nodes;
		sceneGraph.getCollisionNodes(nodes, 0, queryAABB);
		benchmark::DoNotOptimize(nodes);
	}
}

BENCHMARK_DEFINE_F(SceneGraphBenchmark, GetCollisionNodesSparseQuery)(benchmark::State &state) {
	scenegraph::SceneGraph sceneGraph;
	for (int i = 0; i < 64; ++i) {
		scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
		modelNode.createVolume(voxel::Region(0, 0, 0, 3, 3, 3));
		modelNode.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3((float)(i % 8) * 10.0f, 0.0f, (float)(i / 8) * 10.0f));
		modelNode.setTransform(0, transform);
		sceneGraph.emplace(core::move(modelNode));
	}
	sceneGraph.updateTransforms();

	// Tiny query that only overlaps a few nodes - stresses cull path
	const math::AABB<float> queryAABB(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(5.0f, 5.0f, 5.0f));
	for (auto _ : state) {
		scenegraph::CollisionNodes nodes;
		sceneGraph.getCollisionNodes(nodes, 0, queryAABB);
		benchmark::DoNotOptimize(nodes);
	}
}

BENCHMARK_REGISTER_F(SceneGraphBenchmark, Init);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, SceneGraphNode);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, SizeModel);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, GetCollisionNodesMany);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, GetCollisionNodesSparseQuery);

BENCHMARK_MAIN();
