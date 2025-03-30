/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VoxelUtil.h"

class VoxelUtilBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{-20, 20}};
};

BENCHMARK_DEFINE_F(VoxelUtilBenchmark, CopyIntoRegion)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume out(voxel::Region{20, 40});
		voxelutil::copyIntoRegion(v, out, out.region());
	}
}

BENCHMARK_REGISTER_F(VoxelUtilBenchmark, CopyIntoRegion);
