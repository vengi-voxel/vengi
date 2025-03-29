/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/SurfaceExtractor.h"

class RawVolumeWrapperBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
};

BENCHMARK_DEFINE_F(RawVolumeWrapperBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolumeWrapper wrapper(&v);
		wrapper.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_REGISTER_F(RawVolumeWrapperBenchmark, SetVoxel);
