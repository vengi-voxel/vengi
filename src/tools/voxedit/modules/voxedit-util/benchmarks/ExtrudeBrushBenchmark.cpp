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

BENCHMARK_DEFINE_F(ExtrudeBrushBenchmark, PlaceDepth1)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runExtrude(ModifierType::Place, 1);
	}
}

BENCHMARK_DEFINE_F(ExtrudeBrushBenchmark, PlaceDepth5)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runExtrude(ModifierType::Place, 5);
	}
}

BENCHMARK_DEFINE_F(ExtrudeBrushBenchmark, Erase)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runExtrude(ModifierType::Erase, 1);
	}
}

BENCHMARK_REGISTER_F(ExtrudeBrushBenchmark, PlaceDepth1);
BENCHMARK_REGISTER_F(ExtrudeBrushBenchmark, PlaceDepth5);
BENCHMARK_REGISTER_F(ExtrudeBrushBenchmark, Erase);
