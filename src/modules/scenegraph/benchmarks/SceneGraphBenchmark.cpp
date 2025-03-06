/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

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

BENCHMARK_REGISTER_F(SceneGraphBenchmark, Init);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, SceneGraphNode);
BENCHMARK_REGISTER_F(SceneGraphBenchmark, SizeModel);

BENCHMARK_MAIN();
