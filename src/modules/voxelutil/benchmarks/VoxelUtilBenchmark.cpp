/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"

class VoxelUtilBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{-20, 20}};
};

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyIntoRegion)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{-20, 20});
		out.copyInto(v);
	}
}

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyViaRawVolume)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(v, voxel::Region{-20, 20});
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyIntoRegion);
BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyViaRawVolume);
