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

	void fillGaussianSurface(voxel::RawVolume &volume, int size) {
		fillSurface(volume, size);
		const voxel::Voxel v = selectedVoxel();
		for (int y = 2; y <= 5; ++y) {
			volume.setVoxel(0, y, 0, v);
		}
	}

	void fillBridgeGapSurface(voxel::RawVolume &volume) {
		const voxel::Voxel v = selectedVoxel();
		for (int x = -4; x <= 4; ++x) {
			for (int z = -4; z <= 4; ++z) {
				volume.setVoxel(x, 0, z, v);
				volume.setVoxel(x, 4, z, v);
			}
		}
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

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Sculpt)(benchmark::State &state) {
	const voxedit::SculptMode mode = (voxedit::SculptMode)state.range(0);
	for (auto _ : state) {
		brush.onSceneChange();
		if (mode == voxedit::SculptMode::BridgeGap) {
			fillBridgeGapSurface(*node->volume());
		} else if (mode == voxedit::SculptMode::SmoothGaussian) {
			fillGaussianSurface(*node->volume(), _halfSize);
		} else {
			fillSurface(*node->volume(), _halfSize);
		}
		runSculpt(mode);
	}
}

BENCHMARK_REGISTER_F(SculptBrushBenchmark, Sculpt)->DenseRange(0, (int)voxedit::SculptMode::Max - 1);
