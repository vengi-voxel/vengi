/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class SculptBrushBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraphNode *node = nullptr;
	voxedit::SculptBrush brush;

	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, color);
		v.setFlags(voxel::FlagOutline);
		return v;
	}

	void fillSurface(voxel::RawVolume &volume, int size) {
		const voxel::Voxel v = selectedVoxel();
		for (int x = -size; x <= size; ++x) {
			for (int z = -size; z <= size; ++z) {
				volume.setVoxel(x, 0, z, v);
			}
		}
		// Add some surface detail for sculpting
		for (int x = -size / 2; x <= size / 2; ++x) {
			for (int z = -size / 2; z <= size / 2; ++z) {
				volume.setVoxel(x, 1, z, v);
			}
		}
	}

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		const int halfSize = 15;
		const voxel::Region region(-halfSize, halfSize);

		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->setVolume(new voxel::RawVolume(region), true);
		fillSurface(*node->volume(), halfSize);

		brush.init();
	}

	void TearDown(::benchmark::State &state) override {
		brush.shutdown();
		delete node;
		app::AbstractBenchmark::TearDown(state);
	}

	void runLifecycle(voxedit::SculptMode mode) {
		brush.setSculptMode(mode);
		brush.setStrength(0.5f);
		brush.setIterations(3);

		voxedit::BrushContext ctx;
		ctx.modifierType = ModifierType::Override;
		ctx.cursorVoxel = selectedVoxel();
		ctx.cursorFace = voxel::FaceNames::PositiveY;
		ctx.targetVolumeRegion = node->region();

		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		scenegraph::SceneGraph sceneGraph;

		brush.preExecute(ctx, wrapper.volume());
		brush.beginBrush(ctx);
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}
};

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Erode)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), 15);
		runLifecycle(voxedit::SculptMode::Erode);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Grow)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), 15);
		runLifecycle(voxedit::SculptMode::Grow);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, Flatten)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), 15);
		runLifecycle(voxedit::SculptMode::Flatten);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, SmoothAdditive)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), 15);
		runLifecycle(voxedit::SculptMode::SmoothAdditive);
	}
}

BENCHMARK_DEFINE_F(SculptBrushBenchmark, SmoothErode)(benchmark::State &state) {
	for (auto _ : state) {
		brush.onSceneChange();
		fillSurface(*node->volume(), 15);
		runLifecycle(voxedit::SculptMode::SmoothErode);
	}
}

BENCHMARK_REGISTER_F(SculptBrushBenchmark, Erode);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, Grow);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, Flatten);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, SmoothAdditive);
BENCHMARK_REGISTER_F(SculptBrushBenchmark, SmoothErode);
