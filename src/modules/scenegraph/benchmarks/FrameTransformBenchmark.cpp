/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/GLMConst.h"
#include "scenegraph/FrameTransform.h"

class FrameTransformBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(FrameTransformBenchmark, CalcWorldNormal)(benchmark::State &state) {
	for (auto _ : state) {
		scenegraph::FrameTransform transform;
		const glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(state.items_processed(), 0.0f, 0.0f));
		transform.setWorldMatrix(translate);
		benchmark::DoNotOptimize(transform.calcWorldNormal(glm::up()));
	}
}

BENCHMARK_REGISTER_F(FrameTransformBenchmark, CalcWorldNormal);
