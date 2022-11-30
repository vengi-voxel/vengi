/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxelutil/VolumeVisitor.h"

class VoxelVisitorBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{-20, 20}};
public:
	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		const voxel::Voxel voxel = voxel::createVoxel(1);
		v.setVoxel(0, 0, 0, voxel);
		v.setVoxel(0, 0, 2, voxel);
		v.setVoxel(0, 2, 2, voxel);
		v.setVoxel(2, 2, 2, voxel);
		v.setVoxel(0, 2, 0, voxel);
		v.setVoxel(2, 0, 0, voxel);
		v.setVoxel(2, 0, 2, voxel);
		v.setVoxel(2, 2, 0, voxel);
	}
};

static void visitOrder(voxelutil::VisitorOrder order, const voxel::RawVolume &v) {
	auto visitor = [&](int, int, int, const voxel::Voxel &) {};
	const int n = voxelutil::visitVolume(v, visitor, voxelutil::SkipEmpty(), order);
	benchmark::DoNotOptimize(n);
}

BENCHMARK_DEFINE_F(VoxelVisitorBenchmark, Visit)(benchmark::State &state) {
	for (auto _ : state) {
		const voxelutil::VisitorOrder order = (voxelutil::VisitorOrder)(state.range());
		visitOrder(order, v);
	}
}

BENCHMARK_REGISTER_F(VoxelVisitorBenchmark, Visit)->DenseRange(0, (int)(voxelutil::VisitorOrder::Max)-1);

BENCHMARK_MAIN();
