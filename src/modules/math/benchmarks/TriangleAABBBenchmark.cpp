/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "math/TriangleAABB.h"
#include "math/Vec3f.h"

class TriangleAABBBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(TriangleAABBBenchmark, intersectTriangleAABB)(benchmark::State &state) {
	const math::Vec3f half(0.5f);
	const math::Vec3f v0(0.0f, 0.0f, 0.0f);
	const math::Vec3f v1(8.0f, 0.0f, 2.0f);
	const math::Vec3f v2(0.0f, 8.0f, 4.0f);
	int hits = 0;
	for (auto _ : state) {
		hits = 0;
		for (int x = -1; x <= 9; ++x) {
			for (int y = -1; y <= 9; ++y) {
				for (int z = -1; z <= 5; ++z) {
					const math::Vec3f center((float)x, (float)y, (float)z);
					if (math::intersectTriangleAABB(center, half, v0, v1, v2)) {
						++hits;
					}
				}
			}
		}
		benchmark::DoNotOptimize(hits);
	}
}

BENCHMARK_DEFINE_F(TriangleAABBBenchmark, intersectTriangleAABBPrepared)(benchmark::State &state) {
	const math::Vec3f half(0.5f);
	const math::Vec3f v0(0.0f, 0.0f, 0.0f);
	const math::Vec3f v1(8.0f, 0.0f, 2.0f);
	const math::Vec3f v2(0.0f, 8.0f, 4.0f);
	math::TriangleAABBPrep prep;
	math::prepareTriangleAABB(v0, v1, v2, half, prep);
	int hits = 0;
	for (auto _ : state) {
		hits = 0;
		for (int x = -1; x <= 9; ++x) {
			for (int y = -1; y <= 9; ++y) {
				for (int z = -1; z <= 5; ++z) {
					const math::Vec3f center((float)x, (float)y, (float)z);
					if (math::intersectTriangleAABB(center, prep, v0, v1, v2)) {
						++hits;
					}
				}
			}
		}
		benchmark::DoNotOptimize(hits);
	}
}

BENCHMARK_REGISTER_F(TriangleAABBBenchmark, intersectTriangleAABB);
BENCHMARK_REGISTER_F(TriangleAABBBenchmark, intersectTriangleAABBPrepared);
