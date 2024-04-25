/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "math/OBB.h"

class OBBBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(OBBBenchmark, Contains)(benchmark::State &state) {
	for (auto _ : state) {
		math::OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
		benchmark::DoNotOptimize(obb.contains(glm::vec3(0, 0, 0)));
		benchmark::DoNotOptimize(obb.contains(glm::vec3(1, 5, 1)));
	}
}

BENCHMARK_DEFINE_F(OBBBenchmark, Bounds)(benchmark::State &state) {
	for (auto _ : state) {
		math::OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
		benchmark::DoNotOptimize(obb.bounds());
	}
}

BENCHMARK_REGISTER_F(OBBBenchmark, Contains);
BENCHMARK_REGISTER_F(OBBBenchmark, Bounds);

BENCHMARK_MAIN();
