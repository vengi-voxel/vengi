/**
 * @file
 */

#include "BrushBenchmark.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"

class SculptBrushBenchmark : public BrushBenchmark {
protected:
	voxedit::SculptBrush brush;

	void runSculpt(voxedit::SculptMode mode) {
		brush.setSculptMode(mode);
		brush.setStrength(0.5f);
		brush.setIterations(3);
		runBrushLifecycle(brush);
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

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Erode)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runSculpt(voxedit::SculptMode::Erode);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Grow)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runSculpt(voxedit::SculptMode::Grow);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Flatten)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runSculpt(voxedit::SculptMode::Flatten);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, SmoothAdditive)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runSculpt(voxedit::SculptMode::SmoothAdditive);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, SmoothErode)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runSculpt(voxedit::SculptMode::SmoothErode);
	}
}

BENCHMARK_REGISTER_F(SculptBrushBenchmark, Erode);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, Grow);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, Flatten);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, SmoothAdditive);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, SmoothErode);
