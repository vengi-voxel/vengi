/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/VolumeDataAccessor.h"
#include "core/StandardLib.h"

class VolumeDataAccessorBenchmark : public app::AbstractBenchmark {
};

BENCHMARK_DEFINE_F(VolumeDataAccessorBenchmark, accessorSnapshot)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region(-128, 127));
	if (state.range(0) && !volume.voxels()) {
		state.SkipWithError("Could not allocate dense source");
		return;
	}
	if (state.range(2)) {
		for (int z = -128; z <= 127; ++z) {
			for (int y = -128; y <= 127; ++y) {
				for (int x = -128; x <= 127; ++x) {
					volume.setVoxel(x, y, z, voxel::Voxel(voxel::VoxelType::Generic, (uint8_t)(x + 3 * y + 7 * z)));
				}
			}
		}
	}
	const voxel::Region region = state.range(1) ? voxel::Region(-32, 31) : volume.region();
	const size_t bytes = voxel::RawVolume::size(region);
	const size_t sourceBytes = volume.allocatedBytes();
	for (auto _ : state) {
		if (state.range(3) == 0) {
			const voxel::VolumeDataAccessor accessor(volume, region);
			if (!accessor.valid()) {
				state.SkipWithError("Could not create accessor");
				break;
			}
			benchmark::DoNotOptimize(accessor.voxels());
		} else {
			// Reference path: copy each voxel through the volume interface.
			voxel::Voxel *copy = (voxel::Voxel *)core_malloc(bytes);
			if (!copy) {
				state.SkipWithError("Could not allocate reference snapshot");
				break;
			}
			core_memset((void *)copy, 0, bytes);
			size_t i = 0;
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
					for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
						copy[i++] = volume.voxel(x, y, z);
					}
				}
			}
			benchmark::DoNotOptimize(copy);
			core_free(copy);
		}
	}
	state.counters["snapshot_bytes"] = state.range(3) == 0 && volume.isContiguous() && region == volume.region()
		? 0.0 : (double)bytes;
	state.counters["source_allocated_bytes"] = (double)volume.allocatedBytes();
	if (sourceBytes != volume.allocatedBytes()) {
		state.SkipWithError("Snapshot changed source allocation");
	}
}

// dense source, subregion, nonuniform content, per-voxel reference
BENCHMARK_REGISTER_F(VolumeDataAccessorBenchmark, accessorSnapshot)->ArgsProduct({{0, 1}, {0, 1}, {0, 1}, {0, 1}});
