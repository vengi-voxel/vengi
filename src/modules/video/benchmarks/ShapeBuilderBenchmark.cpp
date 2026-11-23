/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "math/OBB.h"
#include "video/ShapeBuilder.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"

class ShapeBuilderBenchmark : public app::AbstractBenchmark {};

BENCHMARK_DEFINE_F(ShapeBuilderBenchmark, OBB)(benchmark::State &state) {
	for (auto _ : state) {
		video::ShapeBuilder shapeBuilder(100);
		const math::Mat4f rot = math::mat4_cast(math::Quatf(math::Vec3f(0.0f, state.range(), 0.0f)));
		math::OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), rot);
		shapeBuilder.obb(obb);
	}
}

BENCHMARK_REGISTER_F(ShapeBuilderBenchmark, OBB)->Arg(0)->Arg(45);

BENCHMARK_MAIN();
