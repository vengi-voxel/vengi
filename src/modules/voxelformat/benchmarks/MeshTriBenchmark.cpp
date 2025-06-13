/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "benchmark/benchmark.h"
#include "glm/ext/vector_float3.hpp"
#include "voxelformat/private/mesh/MeshTri.h"

class MeshTriBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(MeshTriBenchmark, centerUV)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::MeshTri tri;
		tri.setVertices(glm::vec3(0), glm::vec3(1), glm::vec3(3));
		tri.setUVs(glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 0.5f), glm::vec2(1.0f, 1.0f));
		benchmark::DoNotOptimize(tri.centerUV());
	}
}

BENCHMARK_DEFINE_F(MeshTriBenchmark, calcUVs)(benchmark::State &state) {
	for (auto _ : state) {
		voxelformat::MeshTri tri;
		tri.setVertices(glm::vec3(0), glm::vec3(1), glm::vec3(3));
		tri.setUVs(glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 0.5f), glm::vec2(1.0f, 1.0f));
		glm::vec2 out;
		benchmark::DoNotOptimize(tri.calcUVs(glm::vec3(0.0f), out));
		benchmark::DoNotOptimize(out);
	}
}

BENCHMARK_REGISTER_F(MeshTriBenchmark, centerUV);
BENCHMARK_REGISTER_F(MeshTriBenchmark, calcUVs);
