/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeCropper.h"

class VolumeCropperBenchmark : public app::AbstractBenchmark {
};

BENCHMARK_DEFINE_F(VolumeCropperBenchmark, solidBounds)(benchmark::State &state) {
	const glm::ivec3 dimensions((int)state.range(0), (int)state.range(1), (int)state.range(2));
	voxel::RawVolume volume(voxel::Region(glm::ivec3(0), dimensions - 1));
	if (state.range(3) && volume.voxels() == nullptr) {
		state.SkipWithError("Could not allocate dense source");
		return;
	}
	if (state.range(4) == 4) {
		for (int z = 0; z < dimensions.z; ++z) {
			for (int y = 0; y < dimensions.y; ++y) {
				for (int x = 0; x < dimensions.x; ++x) {
					volume.setVoxel(x, y, z, voxel::Voxel(voxel::VoxelType::Generic, (uint8_t)(x + 3 * y + 7 * z)));
				}
			}
		}
	} else if (state.range(4) == 2) {
		volume.fill(voxel::Voxel(voxel::VoxelType::Generic, 2));
	} else if (state.range(4) == 3) {
		volume.fill(voxel::Voxel(voxel::VoxelType::Air, 12, 9, voxel::FlagOutline, 3));
		volume.setVoxel(dimensions / 2, voxel::Voxel(voxel::VoxelType::Generic, 2));
	} else if (state.range(4) == 1) {
		volume.setVoxel(dimensions / 4, voxel::Voxel(voxel::VoxelType::Generic, 2));
		volume.setVoxel(3 * dimensions / 4, voxel::Voxel(voxel::VoxelType::Generic, 2));
	}
	const size_t bytes = volume.allocatedBytes();
	glm::ivec3 mins(0), maxs(0);
	for (auto _ : state) {
		benchmark::DoNotOptimize(voxelutil::computeSolidBounds(&volume, mins, maxs));
		benchmark::DoNotOptimize(mins);
		benchmark::DoNotOptimize(maxs);
	}
	state.counters["allocated_bytes"] = (double)volume.allocatedBytes();
	if (bytes != volume.allocatedBytes()) {
		state.SkipWithError("Bounds query changed source allocation");
	}
}

static void solidBoundsArguments(benchmark::Benchmark *benchmark) {
	const glm::ivec3 sizes[] = {{16, 16, 16}, {16, 256, 16}, {256, 256, 256}};
	for (const glm::ivec3 &size : sizes) {
		for (int dense = 0; dense < 2; ++dense) {
			for (int content = 0; content < 5; ++content) {
				benchmark->Args({size.x, size.y, size.z, dense, content});
			}
		}
	}
}

// Dimensions, force dense, content: empty/sparse/uniform/air metadata/fully allocated.
BENCHMARK_REGISTER_F(VolumeCropperBenchmark, solidBounds)->Apply(solidBoundsArguments);
