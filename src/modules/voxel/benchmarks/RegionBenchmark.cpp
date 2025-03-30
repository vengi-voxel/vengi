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

BENCHMARK_DEFINE_F(RegionBenchmark, ContainsPoint_1)(benchmark::State &state) {
	int i = 0;
	for (auto _ : state) {
		voxel::Region region(0, 0, 0, i, i, i);
		region.containsPoint(1, 2, 3);
		i = (i + 1) % 100;
	}
}

BENCHMARK_DEFINE_F(RegionBenchmark, ContainsPoint_2)(benchmark::State &state) {
	int i = 0;
	for (auto _ : state) {
		voxel::Region region(0, 0, 0, i, i, i);
		region.containsPoint({1, 2, 3, 0});
		i = (i + 1) % 100;
	}
}

BENCHMARK_REGISTER_F(RegionBenchmark, IsValid);
BENCHMARK_REGISTER_F(RegionBenchmark, Accumulate);
BENCHMARK_REGISTER_F(RegionBenchmark, ContainsPoint_1);
BENCHMARK_REGISTER_F(RegionBenchmark, ContainsPoint_2);
