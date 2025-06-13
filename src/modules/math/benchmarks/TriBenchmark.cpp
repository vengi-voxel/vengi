/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "benchmark/benchmark.h"
#include "math/Tri.h"

class TriBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(TriBenchmark, calculateBarycentric)(benchmark::State &state) {
	for (auto _ : state) {
		math::Tri tri;
		tri.setVertices(glm::vec3(0), glm::vec3(1), glm::vec3(3));
		benchmark::DoNotOptimize(tri.calculateBarycentric(tri.center()));
	}
}

BENCHMARK_DEFINE_F(TriBenchmark, center)(benchmark::State &state) {
	for (auto _ : state) {
		math::Tri tri;
		tri.setVertices(glm::vec3(0), glm::vec3(1), glm::vec3(3));
		benchmark::DoNotOptimize(tri.center());
	}
}

BENCHMARK_REGISTER_F(TriBenchmark, calculateBarycentric);
BENCHMARK_REGISTER_F(TriBenchmark, center);
