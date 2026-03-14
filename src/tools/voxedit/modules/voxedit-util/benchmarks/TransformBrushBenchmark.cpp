/**
 * @file
 */

#include "BrushBenchmark.h"
#include "voxedit-util/modifier/brush/TransformBrush.h"

class TransformBrushBenchmark : public BrushBenchmark {
protected:
	voxedit::TransformBrush brush;

	void runTransform(voxedit::TransformMode mode) {
		brush.setTransformMode(mode);
		brush.setVoxelSampling(voxel::VoxelSampling::Cubic);

		switch (mode) {
		case voxedit::TransformMode::Move:
			brush.setMoveOffset(glm::ivec3(3, 0, 2));
			break;
		case voxedit::TransformMode::Shear:
			brush.setShearOffset(glm::ivec3(2, 0, 0));
			break;
		case voxedit::TransformMode::Scale:
			brush.setScale(glm::vec3(1.5f));
			break;
		case voxedit::TransformMode::Rotate:
			brush.setRotationDegrees(glm::vec3(0.0f, 45.0f, 0.0f));
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

BENCHMARK_DEFINE_F(TransformBrushBenchmark, Move)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runTransform(voxedit::TransformMode::Move);
	}
}

BENCHMARK_DEFINE_F(TransformBrushBenchmark, Shear)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runTransform(voxedit::TransformMode::Shear);
	}
}

BENCHMARK_DEFINE_F(TransformBrushBenchmark, Scale)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runTransform(voxedit::TransformMode::Scale);
	}
}

BENCHMARK_DEFINE_F(TransformBrushBenchmark, Rotate)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), _halfSize);
		runTransform(voxedit::TransformMode::Rotate);
	}
}

BENCHMARK_REGISTER_F(TransformBrushBenchmark, Move);
BENCHMARK_REGISTER_F(TransformBrushBenchmark, Shear);
BENCHMARK_REGISTER_F(TransformBrushBenchmark, Scale);
BENCHMARK_REGISTER_F(TransformBrushBenchmark, Rotate);
