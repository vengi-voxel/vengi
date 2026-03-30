/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MeshState.h"
#include "voxel/RawVolume.h"
#include "voxelrender/SceneGraphRenderer.h"

/**
 * @brief Benchmark-only class that inherits from SceneGraphRenderer to access protected methods
 */
class BenchmarkableSceneGraphRenderer : public voxelrender::SceneGraphRenderer {
public:
	using SceneGraphRenderer::prepareMeshStateTransform;
};

class SceneGraphRendererBenchmark : public app::AbstractBenchmark {
protected:
	static constexpr int NumNodes = 100;
	voxel::MeshStatePtr _meshState;
	scenegraph::SceneGraph _sceneGraph;
	BenchmarkableSceneGraphRenderer _renderer;
	core::Buffer<voxel::RawVolume *> _volumes;
	core::Buffer<int> _nodeIds;

public:
	bool onInitApp() override {
		if (!app::AbstractBenchmark::onInitApp()) {
			return false;
		}
		_meshState = core::make_shared<voxel::MeshState>();
		_meshState->construct();
		if (!_meshState->init()) {
			return false;
		}
		return true;
	}

	void onCleanupApp() override {
		if (_meshState) {
			(void)_meshState->shutdown();
			_meshState = voxel::MeshStatePtr();
		}
		for (voxel::RawVolume *v : _volumes) {
			delete v;
		}
		_volumes.clear();
		app::AbstractBenchmark::onCleanupApp();
	}

	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		_sceneGraph = scenegraph::SceneGraph();
		_nodeIds.clear();

		for (int i = 0; i < NumNodes; ++i) {
			voxel::RawVolume *vol = new voxel::RawVolume(voxel::Region{0, 0, 0, 31, 31, 31});
			vol->setVoxel(10, 10, 10, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			_volumes.push_back(vol);

			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setUnownedVolume(vol);
			node.setPivot(glm::vec3(0.5f));
			int nodeId = _sceneGraph.emplace(core::move(node), 0);
			_nodeIds.push_back(nodeId);

			int idx = _renderer.getOrAssignVolumeIdx(nodeId);
			bool meshDeleted = false;
			(void)_meshState->setVolume(idx, vol, nullptr, nullptr, false, meshDeleted);
		}
		_sceneGraph.updateTransforms();
	}
};

BENCHMARK_DEFINE_F(SceneGraphRendererBenchmark, PrepareMeshStateTransform)(benchmark::State &state) {
	const scenegraph::FrameIndex frame = 0;
	for (auto _ : state) {
		for (int i = 0; i < NumNodes; ++i) {
			const int nodeId = _nodeIds[i];
			const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
			const int idx = _renderer.getVolumeIdx(nodeId);
			_renderer.prepareMeshStateTransform(_meshState, _sceneGraph, frame, node, idx);
		}
	}
}

BENCHMARK_DEFINE_F(SceneGraphRendererBenchmark, PrepareMeshStateTransformSingle)(benchmark::State &state) {
	const scenegraph::FrameIndex frame = 0;
	const int nodeId = _nodeIds[0];
	const scenegraph::SceneGraphNode &node = _sceneGraph.node(nodeId);
	const int idx = _renderer.getVolumeIdx(nodeId);
	for (auto _ : state) {
		_renderer.prepareMeshStateTransform(_meshState, _sceneGraph, frame, node, idx);
	}
}

BENCHMARK_REGISTER_F(SceneGraphRendererBenchmark, PrepareMeshStateTransform);
BENCHMARK_REGISTER_F(SceneGraphRendererBenchmark, PrepareMeshStateTransformSingle);

BENCHMARK_MAIN();
