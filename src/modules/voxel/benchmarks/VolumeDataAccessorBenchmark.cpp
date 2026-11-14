/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/StandardLib.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeDataAccessor.h"

class VolumeDataAccessorBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(VolumeDataAccessorBenchmark, accessorSnapshot)(benchmark::State &state) {
	voxel::RawVolume volume(voxel::Region(-64, 63));
	if (!volume.valid()) {
		state.SkipWithError("Could not allocate dense source");
		return;
	}
	if (state.range(1)) {
		volume.fill(voxel::Voxel(voxel::VoxelType::Generic, 3));
	}
	const voxel::Region region = state.range(0) ? voxel::Region(-16, 15) : volume.region();
	for (auto _ : state) {
		const voxel::VolumeDataAccessor accessor(volume, region);
		if (!accessor.valid()) {
			state.SkipWithError("Could not create accessor");
			break;
		}
		benchmark::DoNotOptimize(accessor.voxels());
	}
	state.counters["snapshot_bytes"] =
		region == volume.region() ? 0.0 : (double)voxel::RawVolume::size(region);
}

BENCHMARK_REGISTER_F(VolumeDataAccessorBenchmark, accessorSnapshot)->ArgsProduct({{0, 1}, {0, 1}});
