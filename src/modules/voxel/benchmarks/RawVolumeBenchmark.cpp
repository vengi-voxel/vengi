/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"

class RawVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
};

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, IsEmpty)(benchmark::State &state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(v.isEmpty(v.region()));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::RawVolume::Sampler sampler(&v);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(RawVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, IsEmpty);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(RawVolumeBenchmark, SetVoxels);
