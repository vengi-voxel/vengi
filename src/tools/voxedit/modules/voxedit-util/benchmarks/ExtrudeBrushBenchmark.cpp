/**
 * @file
 */

#include "BrushBenchmark.h"
#include "voxedit-util/modifier/brush/ExtrudeBrush.h"

class ExtrudeBrushBenchmark : public BrushBenchmark {
protected:
	voxedit::ExtrudeBrush brush;

	void runExtrude(ModifierType modifierType, int depth) {
		brush.setDepth(depth);
		runBrushLifecycle(brush, modifierType, voxel::FaceNames::PositiveY);
	}

public:
	void SetUp(::benchmark::State &state) override {
		BrushBenchmark::SetUp(state);
		brush.init();
	}

	void TearDown(::benchmark::State &state) override {
		brush.shutdown();
		BrushBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(ExtrudeBrushBenchmark, Extrude)(benchmark::State &state) {
	const ModifierType modifierType = (ModifierType)state.range(0);
	const int depth = (int)state.range(1);
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runExtrude(modifierType, depth);
	}
}

BENCHMARK_REGISTER_F(ExtrudeBrushBenchmark, Extrude)
	->Args({(int)ModifierType::Place, 1})
	->Args({(int)ModifierType::Place, 5})
	->Args({(int)ModifierType::Erase, 1});
