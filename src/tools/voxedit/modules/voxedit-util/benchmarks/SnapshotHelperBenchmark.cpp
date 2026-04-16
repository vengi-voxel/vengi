/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/SnapshotHelper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

class SnapshotHelperBenchmark : public app::AbstractBenchmark {
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

public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		const voxel::Region region(-_halfSize, _halfSize);
		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->createVolume(region);
		fillSurface(*node->volume(), _halfSize);
	}

	void TearDown(::benchmark::State &state) override {
		delete node;
		node = nullptr;
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(SnapshotHelperBenchmark, CaptureSnapshot)(benchmark::State &state) {
	for (auto _ : state) {
		voxedit::SnapshotHelper helper;
		helper.captureSnapshot(node->volume(), node->region());
		benchmark::DoNotOptimize(helper.snapshotVoxelCount());
	}
}

BENCHMARK_DEFINE_F(SnapshotHelperBenchmark, WriteAndRevert)(benchmark::State &state) {
	for (auto _ : state) {
		state.PauseTiming();
		// Reset volume
		voxel::RawVolume *vol = node->volume();
		const voxel::Region &region = vol->region();
		const glm::ivec3 &lo = region.getLowerCorner();
		const glm::ivec3 &hi = region.getUpperCorner();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					vol->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
		fillSurface(*vol, _halfSize);

		voxedit::SnapshotHelper helper;
		helper.captureSnapshot(vol, region);

		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		state.ResumeTiming();

		// Write voxels at shifted positions
		const voxel::Voxel newVoxel = selectedVoxel(5);
		for (int x = -_halfSize; x <= _halfSize; ++x) {
			for (int z = -_halfSize; z <= _halfSize; ++z) {
				helper.writeVoxel(wrapper, glm::ivec3(x, 2, z), newVoxel);
			}
		}

		// Revert all changes
		voxel::Region dirty = helper.revertChanges(vol);
		benchmark::DoNotOptimize(dirty);
	}
}

BENCHMARK_DEFINE_F(SnapshotHelperBenchmark, RestoreHistory)(benchmark::State &state) {
	for (auto _ : state) {
		state.PauseTiming();
		voxel::RawVolume *vol = node->volume();
		const voxel::Region &region = vol->region();
		const glm::ivec3 &lo = region.getLowerCorner();
		const glm::ivec3 &hi = region.getUpperCorner();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					vol->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
		fillSurface(*vol, _halfSize);

		voxedit::SnapshotHelper helper;
		helper.captureSnapshot(vol, region);

		voxedit::ModifierVolumeWrapper wrapper(*node, ModifierType::Override);

		const voxel::Voxel newVoxel = selectedVoxel(5);
		for (int x = -_halfSize; x <= _halfSize; ++x) {
			for (int z = -_halfSize; z <= _halfSize; ++z) {
				helper.writeVoxel(wrapper, glm::ivec3(x, 2, z), newVoxel);
			}
		}
		state.ResumeTiming();

		helper.restoreHistory(vol, wrapper);
		voxel::Voxel voxel = vol->voxel(0, 0, 0);
		benchmark::DoNotOptimize(voxel);
	}
}

BENCHMARK_DEFINE_F(SnapshotHelperBenchmark, AdjustForRegionShift)(benchmark::State &state) {
	voxedit::SnapshotHelper helper;
	helper.captureSnapshot(node->volume(), node->region());

	int shift = 1;
	for (auto _ : state) {
		helper.adjustForRegionShift(glm::ivec3(shift, 0, 0));
		shift = -shift;
		voxel::Region region = helper.snapshotRegion();
		benchmark::DoNotOptimize(region);
	}
}

BENCHMARK_REGISTER_F(SnapshotHelperBenchmark, CaptureSnapshot);
BENCHMARK_REGISTER_F(SnapshotHelperBenchmark, WriteAndRevert);
BENCHMARK_REGISTER_F(SnapshotHelperBenchmark, RestoreHistory);
BENCHMARK_REGISTER_F(SnapshotHelperBenchmark, AdjustForRegionShift);
