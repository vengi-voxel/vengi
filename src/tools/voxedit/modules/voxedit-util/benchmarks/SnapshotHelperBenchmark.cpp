/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/SnapshotHelper.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

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

/**
 * @brief Large-volume benchmark simulating a 512x512x256 volume with a circle
 * selection of radius 80 (~20K selected voxels out of 67M total).
 * Uses threadPoolSize=0 (auto-detect CPU count) to properly benchmark parallel code.
 */
class SnapshotHelperLargeBenchmark : public app::AbstractBenchmark {
protected:
	scenegraph::SceneGraphNode *node = nullptr;

	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, color);
		v.setFlags(voxel::FlagOutline);
		return v;
	}

public:
	SnapshotHelperLargeBenchmark() : app::AbstractBenchmark(4) {
	}

	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		const voxel::Region region(0, 0, 0, 511, 511, 255);
		node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		node->createVolume(region);
		voxel::RawVolume *vol = node->volume();
		// Fill a flat layer at y=0
		const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		for (int x = 0; x < 512; ++x) {
			for (int z = 0; z < 512; ++z) {
				vol->setVoxel(x, 0, z, solid);
			}
		}
		// Select a circle of radius 80 centered at (256, 0, 256)
		const voxel::Voxel sel = selectedVoxel();
		const int radius = 80;
		const int cx = 256, cz = 256;
		for (int x = cx - radius; x <= cx + radius; ++x) {
			for (int z = cz - radius; z <= cz + radius; ++z) {
				const int dx = x - cx;
				const int dz = z - cz;
				if (dx * dx + dz * dz <= radius * radius) {
					vol->setVoxel(x, 0, z, sel);
				}
			}
		}
	}

	void TearDown(::benchmark::State &state) override {
		delete node;
		node = nullptr;
		app::AbstractBenchmark::TearDown(state);
	}
};

BENCHMARK_DEFINE_F(SnapshotHelperLargeBenchmark, CaptureSnapshot)(benchmark::State &state) {
	for (auto _ : state) {
		voxedit::SnapshotHelper helper;
		helper.captureSnapshot(node->volume(), node->region());
		benchmark::DoNotOptimize(helper.snapshotVoxelCount());
	}
}

// Sequential baseline: uses visitVolume (non-parallel) + one-by-one setVoxel
BENCHMARK_DEFINE_F(SnapshotHelperLargeBenchmark, CaptureSnapshotSequential)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::SparseVolume snapshot;
		glm::ivec3 selLo(node->region().getUpperCorner());
		glm::ivec3 selHi(node->region().getLowerCorner());
		voxelutil::visitVolume(
			*node->volume(), node->region(),
			[&](int x, int y, int z, const voxel::Voxel &voxel) {
				const glm::ivec3 pos(x, y, z);
				snapshot.setVoxel(pos, voxel);
				selLo = glm::min(selLo, pos);
				selHi = glm::max(selHi, pos);
			},
			voxelutil::VisitSolidOutline());
		benchmark::DoNotOptimize(snapshot.size());
	}
}

BENCHMARK_REGISTER_F(SnapshotHelperLargeBenchmark, CaptureSnapshot)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SnapshotHelperLargeBenchmark, CaptureSnapshotSequential)->Unit(benchmark::kMillisecond);
