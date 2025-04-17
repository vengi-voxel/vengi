#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxelgenerator/ShapeGenerator.h"

class ShapeGeneratorBenchmark : public app::AbstractBenchmark {
private:
	using Super = app::AbstractBenchmark;
};

BENCHMARK_DEFINE_F(ShapeGeneratorBenchmark, circle)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolume v{voxel::Region(-20, 20)};
		voxelgenerator::shape::createCirclePlane(v, glm::vec3(0, 0, 0), math::Axis::X, 10, 10, 4.0, voxel::Voxel());
	}
}

BENCHMARK_REGISTER_F(ShapeGeneratorBenchmark, circle);
BENCHMARK_MAIN();
