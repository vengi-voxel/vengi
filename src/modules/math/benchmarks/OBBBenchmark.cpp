/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "math/OBB.h"
#include "math/Mat3f.h"
#include "math/Vec3f.h"

class OBBBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(OBBBenchmark, Contains)(benchmark::State &state) {
	for (auto _ : state) {
		math::OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
		benchmark::DoNotOptimize(obb.contains(math::Vec3f(0, 0, 0)));
		benchmark::DoNotOptimize(obb.contains(math::Vec3f(1, 5, 1)));
	}
}

BENCHMARK_DEFINE_F(OBBBenchmark, Intersects)(benchmark::State &state) {
	for (auto _ : state) {
		math::OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
		float distance = 0.0f;
		benchmark::DoNotOptimize(obb.intersect(math::Vec3f(10.0f, 0.0, 0.0f), math::Vec3f(-1.0f, 0.0f, 0.0f), distance));
	}
}

BENCHMARK_REGISTER_F(OBBBenchmark, Contains);
BENCHMARK_REGISTER_F(OBBBenchmark, Intersects);

BENCHMARK_MAIN();
