/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/GLM.h"
#include "math/OBB.h"
#include "video/ShapeBuilder.h"
#include <glm/gtc/quaternion.hpp>

class ShapeBuilderBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(ShapeBuilderBenchmark, OBB)(benchmark::State &state) {
	for (auto _ : state) {
		video::ShapeBuilder shapeBuilder(100);
		const glm::mat4 rot = glm::mat4_cast(glm::quat(glm::vec3(0.0f, state.range(), 0.0f)));
		math::OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), rot);
		shapeBuilder.obb(obb);
	}
}

BENCHMARK_REGISTER_F(ShapeBuilderBenchmark, OBB)->Arg(0)->Arg(45);

BENCHMARK_MAIN();
