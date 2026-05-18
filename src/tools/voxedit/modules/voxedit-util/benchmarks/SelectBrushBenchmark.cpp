/**
 * @file
 */

#include "BrushBenchmark.h"
#include "voxedit-util/modifier/brush/SelectBrush.h"
#include "voxedit-util/modifier/brush/select/All.h"
#include "voxedit-util/modifier/brush/select/Circle.h"
#include "voxedit-util/modifier/brush/select/Connected.h"
#include "voxedit-util/modifier/brush/select/Surface.h"
#include "voxelutil/VolumeSelect.h"

class SelectBrushBenchmark : public BrushBenchmark {
protected:
	voxedit::SelectBrush brush{nullptr};

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

/**
 * @brief Benchmark the point-in-polygon test used by the lasso strategy.
 * This isolates the polygon containment check which is called per-pixel.
 */
class LassoPointInPolygonBenchmark : public app::AbstractBenchmark {
};

/**
 * @brief Benchmark isolated strategy generate() calls to establish baselines
 * for comparison
 */
class SelectStrategyBenchmark : public BrushBenchmark {
};

BENCHMARK_DEFINE_F(SelectBrushBenchmark, All)(benchmark::State &state) {
	brush.setSelectMode(voxedit::SelectMode::All);
	for (auto _ : state) {
		// Clear selection flags before each iteration
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		runBrushLifecycle(brush);
	}
}

BENCHMARK_DEFINE_F(SelectBrushBenchmark, Surface)(benchmark::State &state) {
	brush.setSelectMode(voxedit::SelectMode::Surface);
	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		runBrushLifecycle(brush);
	}
}

BENCHMARK_DEFINE_F(SelectBrushBenchmark, Connected)(benchmark::State &state) {
	brush.setSelectMode(voxedit::SelectMode::Connected);
	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		runBrushLifecycle(brush);
	}
}

BENCHMARK_DEFINE_F(LassoPointInPolygonBenchmark, SquarePolygon)(benchmark::State &state) {
	const int numPoints = state.range(0);
	// Build a circular polygon with numPoints vertices
	core::DynamicArray<glm::ivec3> path;
	path.reserve(numPoints);
	const float radius = 50.0f;
	for (int i = 0; i < numPoints; ++i) {
		float angle = 2.0f * 3.14159265f * (float)i / (float)numPoints;
		int x = (int)(radius * cosf(angle) + 50.0f);
		int z = (int)(radius * sinf(angle) + 50.0f);
		path.push_back(glm::ivec3(x, 0, z));
	}

	const int uAxis = 0;
	const int vAxis = 2;

	for (auto _ : state) {
		// Test 100x100 grid of points against the polygon
		for (int y = 0; y < 100; ++y) {
			for (int x = 0; x < 100; ++x) {
				benchmark::DoNotOptimize(voxelutil::lassoContains(path, x, y, uAxis, vAxis));
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 10000);
}

BENCHMARK_DEFINE_F(SelectStrategyBenchmark, AllGenerate)(benchmark::State &state) {
	voxedit::select::All strategy;
	scenegraph::SceneGraph sceneGraph;
	voxedit::BrushContext ctx;
	ctx.targetVolumeRegion = node->region();
	voxedit::select::AABBBrushState brushState;

	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		strategy.generate(sceneGraph, wrapper, ctx, node->region(), brushState);
	}
}

BENCHMARK_DEFINE_F(SelectStrategyBenchmark, SurfaceGenerate)(benchmark::State &state) {
	voxedit::select::Surface strategy;
	scenegraph::SceneGraph sceneGraph;
	voxedit::BrushContext ctx;
	ctx.targetVolumeRegion = node->region();
	voxedit::select::AABBBrushState brushState;

	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		strategy.generate(sceneGraph, wrapper, ctx, node->region(), brushState);
	}
}

BENCHMARK_DEFINE_F(SelectStrategyBenchmark, ConnectedGenerate)(benchmark::State &state) {
	voxedit::select::Connected strategy;
	scenegraph::SceneGraph sceneGraph;
	voxedit::BrushContext ctx;
	ctx.targetVolumeRegion = node->region();
	ctx.cursorPosition = glm::ivec3(0, 0, 0);
	ctx.hitCursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxedit::select::AABBBrushState brushState;

	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		strategy.generate(sceneGraph, wrapper, ctx, node->region(), brushState);
	}
}

BENCHMARK_REGISTER_F(SelectStrategyBenchmark, ConnectedGenerate);
BENCHMARK_REGISTER_F(SelectStrategyBenchmark, SurfaceGenerate);
BENCHMARK_REGISTER_F(SelectStrategyBenchmark, AllGenerate);
BENCHMARK_REGISTER_F(LassoPointInPolygonBenchmark, SquarePolygon)->Arg(4)->Arg(20)->Arg(100)->Arg(500);
BENCHMARK_REGISTER_F(SelectBrushBenchmark, Connected);
BENCHMARK_REGISTER_F(SelectBrushBenchmark, Surface);
BENCHMARK_REGISTER_F(SelectBrushBenchmark, All);

/**
 * @brief Large-volume benchmark for Circle selection on a 512x512x256 volume.
 * Simulates selecting a circle of radius 80 on a flat surface.
 */
class CircleSelectLargeBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraphNode *node = nullptr;

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		const voxel::Region region(0, 0, 0, 511, 511, 255);
		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->createVolume(region);
		voxel::RawVolume *vol = node->volume();
		const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		for (int x = 0; x < 512; ++x) {
			for (int z = 0; z < 512; ++z) {
				vol->setVoxel(x, 0, z, solid);
			}
		}
	}

	void TearDown(::benchmark::State &state) override {
		delete node;
		node = nullptr;
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(CircleSelectLargeBenchmark, Radius80)(benchmark::State &state) {
	voxedit::select::Circle strategy;
	scenegraph::SceneGraph sceneGraph;
	voxedit::BrushContext ctx;
	ctx.targetVolumeRegion = node->region();
	ctx.cursorPosition = glm::ivec3(256 + 80, 0, 256);
	voxedit::select::AABBBrushState brushState;
	brushState.boxMode = true;
	brushState.aabbFace = voxel::FaceNames::PositiveY;
	brushState.aabbFirstPos = glm::ivec3(256, 0, 256);
	brushState.cursorPosition = ctx.cursorPosition;

	const voxel::Region circleRegion = strategy.calcRegion(ctx, brushState);

	for (auto _ : state) {
		node->volume()->removeFlags(node->region(), voxel::FlagOutline);
		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		strategy.generate(sceneGraph, wrapper, ctx, circleRegion, brushState);
	}
}

BENCHMARK_REGISTER_F(CircleSelectLargeBenchmark, Radius80)->Unit(benchmark::kMillisecond);
