/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "benchmark/benchmark.h"
#include "voxelformat/private/mesh/MeshTri.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

class MeshTriBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(MeshTriBenchmark, centerUV)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::MeshTri tri;
		tri.setVertices(math::Vec3f(0), math::Vec3f(1), math::Vec3f(3));
		tri.setUVs(math::Vec2f(0.0f, 0.0f), math::Vec2f(0.5f, 0.5f), math::Vec2f(1.0f, 1.0f));
		benchmark::DoNotOptimize(tri.centerUV());
	}
}

BENCHMARK_DEFINE_F(MeshTriBenchmark, calcUVs)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::MeshTri tri;
		tri.setVertices(math::Vec3f(0), math::Vec3f(1), math::Vec3f(3));
		tri.setUVs(math::Vec2f(0.0f, 0.0f), math::Vec2f(0.5f, 0.5f), math::Vec2f(1.0f, 1.0f));
		math::Vec2f out;
		benchmark::DoNotOptimize(tri.calcUVs(math::Vec3f(0.0f), out));
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_REGISTER_F(MeshTriBenchmark, centerUV);
BENCHMARK_REGISTER_F(MeshTriBenchmark, calcUVs);
