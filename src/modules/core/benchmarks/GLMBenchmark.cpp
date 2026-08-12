/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/GLM.h"

class GLMBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(GLMBenchmark, intersectTriangleAABB)(benchmark::State &state) {
	const glm::vec3 half(0.5f);
	const glm::vec3 v0(0.0f, 0.0f, 0.0f);
	const glm::vec3 v1(8.0f, 0.0f, 2.0f);
	const glm::vec3 v2(0.0f, 8.0f, 4.0f);
	int hits = 0;
	for (auto _ : state) {
		hits = 0;
		for (int x = -1; x <= 9; ++x) {
			for (int y = -1; y <= 9; ++y) {
				for (int z = -1; z <= 5; ++z) {
					const glm::vec3 center((float)x, (float)y, (float)z);
					if (glm::intersectTriangleAABB(center, half, v0, v1, v2)) {
						++hits;
					}
				}
			}
		}
		benchmark::DoNotOptimize(hits);
	}
}

BENCHMARK_DEFINE_F(GLMBenchmark, intersectTriangleAABBPrepared)(benchmark::State &state) {
	const glm::vec3 half(0.5f);
	const glm::vec3 v0(0.0f, 0.0f, 0.0f);
	const glm::vec3 v1(8.0f, 0.0f, 2.0f);
	const glm::vec3 v2(0.0f, 8.0f, 4.0f);
	glm::TriangleAABBPrep prep;
	glm::prepareTriangleAABB(v0, v1, v2, half, prep);
	int hits = 0;
	for (auto _ : state) {
		hits = 0;
		for (int x = -1; x <= 9; ++x) {
			for (int y = -1; y <= 9; ++y) {
				for (int z = -1; z <= 5; ++z) {
					const glm::vec3 center((float)x, (float)y, (float)z);
					if (glm::intersectTriangleAABB(center, prep, v0, v1, v2)) {
						++hits;
					}
				}
			}
		}
		benchmark::DoNotOptimize(hits);
	}
}

BENCHMARK_REGISTER_F(GLMBenchmark, intersectTriangleAABB);
BENCHMARK_REGISTER_F(GLMBenchmark, intersectTriangleAABBPrepared);
