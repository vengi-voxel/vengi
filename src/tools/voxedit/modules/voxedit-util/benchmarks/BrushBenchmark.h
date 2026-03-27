/**
 * @file
 */

#pragma once

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class BrushBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraphNode *node = nullptr;
	int _halfSize = 15;

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
		for (int x = -size / 2; x <= size / 2; ++x) {
			for (int z = -size / 2; z <= size / 2; ++z) {
				volume.setVoxel(x, 1, z, v);
			}
		}
	}

	void createNode(int halfSize) {
		_halfSize = halfSize;
		const voxel::Region region(-halfSize, halfSize);
		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->setVolume(new voxel::RawVolume(region));
		fillSurface(*node->volume(), halfSize);
	}

	void runBrushLifecycle(voxedit::Brush &brush, ModifierType modifierType = ModifierType::Override,
						   voxel::FaceNames face = voxel::FaceNames::PositiveY) {
		voxedit::BrushContext ctx;
		ctx.modifierType = modifierType;
		ctx.cursorVoxel = selectedVoxel();
		ctx.cursorFace = face;
		ctx.targetVolumeRegion = node->region();

		voxedit::ModifierVolumeWrapper wrapper(*node, modifierType);
		scenegraph::SceneGraph sceneGraph;

		brush.preExecute(ctx, wrapper.volume());
		brush.beginBrush(ctx);
		brush.execute(sceneGraph, wrapper, ctx);
		brush.endBrush(ctx);
	}

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		createNode(15);
	}

	void TearDown(::benchmark::State &state) override {
		delete node;
		node = nullptr;
		app::AbstractBenchmark::TearDown(state);
	}
};
