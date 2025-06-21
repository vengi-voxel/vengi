/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Vector.h"
#include "voxel/SparseVolume.h"

class SparseVolumeBenchmark : public app::AbstractBenchmark {
protected:
	voxel::SparseVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
	voxel::SparseVolume vunlimit{};
};

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, CalculateRegion)(benchmark::State &state) {
	for (auto _ : state) {
		for (int i = 0; i < 10; ++i)
			v.setVoxel(96, i, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		benchmark::DoNotOptimize(v.calculateRegion());
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::SparseVolume::Sampler sampler(&v);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxel_unlimit)(benchmark::State &state) {
	for (auto _ : state) {
		vunlimit.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		vunlimit.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelSampler_unlimit)(benchmark::State &state) {
	voxel::SparseVolume::Sampler sampler(&vunlimit);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(SparseVolumeBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxel_unlimit);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelSampler_unlimit);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, CalculateRegion);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(SparseVolumeBenchmark, SetVoxels);
