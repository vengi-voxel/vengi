/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/Clipper.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class ClipperBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraph _sceneGraph;
	voxel::RawVolume _v{{-10, 10}};
	scenegraph::Clipper _clipper{_sceneGraph};

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		_sceneGraph = scenegraph::SceneGraph();

		scenegraph::SceneGraphNode node{scenegraph::SceneGraphNodeType::Model};
		_v.setVoxel(-2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_v.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_v.setVoxel(4, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		node.setVolume(&_v, false);
		_sceneGraph.emplace(core::move(node));
	}
};

BENCHMARK_DEFINE_F(ClipperBenchmark, ClipNoFrame)(benchmark::State &state) {
	for (auto _ : state) {
		const glm::vec3 worldPos(0.0f, 0.0f, 0.0f);
		const glm::vec3 dir(1.0f, 0.0f, 0.0f);
		const glm::mat3 noRot(1.0f);
		benchmark::DoNotOptimize(_clipper.clipDelta(InvalidFrame, worldPos, dir, noRot));
	}
}

BENCHMARK_DEFINE_F(ClipperBenchmark, ClipFrame)(benchmark::State &state) {
	for (auto _ : state) {
		const glm::vec3 worldPos(0.0f, 0.0f, 0.0f);
		const glm::vec3 dir(1.0f, 0.0f, 0.0f);
		const glm::mat3 noRot(1.0f);
		benchmark::DoNotOptimize(_clipper.clipDelta(0, worldPos, dir, noRot));
	}
}

BENCHMARK_REGISTER_F(ClipperBenchmark, ClipNoFrame);
BENCHMARK_REGISTER_F(ClipperBenchmark, ClipFrame);
