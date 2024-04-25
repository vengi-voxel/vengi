/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "math/OBB.h"
#include "video/ShapeBuilder.h"

class ShapeBuilderBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(ShapeBuilderBenchmark, OBB)(benchmark::State &state) {
	for (auto _ : state) {
		video::ShapeBuilder shapeBuilder(100);
		math::OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
		shapeBuilder.obb(obb);
	}
}

BENCHMARK_REGISTER_F(ShapeBuilderBenchmark, OBB);

BENCHMARK_MAIN();
