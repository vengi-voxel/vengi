/**
 * @file
 */

#include "BrushBenchmark.h"
#include "voxedit-util/modifier/brush/TransformBrush.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

class TransformBrushBenchmark : public BrushBenchmark {
protected:
	voxedit::TransformBrush brush;

	void runTransform(voxedit::TransformMode mode) {
		brush.setTransformMode(mode);
		brush.setVoxelSampling(voxel::VoxelSampling::Cubic);

		switch (mode) {
		case voxedit::TransformMode::Move:
			brush.setMoveOffset(math::Vec3i(3, 0, 2));
			break;
		case voxedit::TransformMode::Shear:
			brush.setShearOffset(math::Vec3i(2, 0, 0));
			break;
		case voxedit::TransformMode::Scale:
			brush.setScale(math::Vec3f(1.5f));
			break;
		case voxedit::TransformMode::Rotate:
			brush.setRotationDegrees(math::Vec3f(0.0f, 45.0f, 0.0f));
			break;
		default:
			break;
		}

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

BENCHMARK_DEFINE_F(TransformBrushBenchmark, Transform)(benchmark::State &state) {
	const voxedit::TransformMode mode = (voxedit::TransformMode)state.range(0);
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runTransform(mode);
	}
}

BENCHMARK_REGISTER_F(TransformBrushBenchmark, Transform)->DenseRange(0, (int)voxedit::TransformMode::Max - 1);
