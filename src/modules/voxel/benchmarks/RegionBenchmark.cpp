/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/Region.h"

class RegionBenchmark : public app::AbstractBenchmark {
};

BENCHMARK_DEFINE_F(RegionBenchmark, IsValid)(benchmark::State &state) {
	voxel::Region region;
	for (auto _ : state) {
		benchmark::DoNotOptimize(region.isValid());
	}
}

BENCHMARK_DEFINE_F(RegionBenchmark, Accumulate)(benchmark::State &state) {
	voxel::Region region(0, 0, 0, 0, 0, 0);
	for (auto _ : state) {
		region.accumulate(1, 2, 3);
	}
}

BENCHMARK_REGISTER_F(RegionBenchmark, IsValid);
BENCHMARK_REGISTER_F(RegionBenchmark, Accumulate);
