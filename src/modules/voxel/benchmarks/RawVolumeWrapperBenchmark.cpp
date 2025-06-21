/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/VolumeSamplerUtil.h"

class RawVolumeWrapperBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
};

BENCHMARK_DEFINE_F(RawVolumeWrapperBenchmark, SetVoxel)(benchmark::State &state) {
	for (auto _ : state) {
		voxel::RawVolumeWrapper wrapper(&v);
		wrapper.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		wrapper.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeWrapperBenchmark, SetVoxelSampler)(benchmark::State &state) {
	voxel::RawVolumeWrapper wrapper(&v);
	voxel::RawVolumeWrapper::Sampler sampler(&wrapper);
	for (auto _ : state) {
		sampler.setPosition(96, 6, 62);
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		sampler.movePositiveY();
		sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
}

BENCHMARK_DEFINE_F(RawVolumeWrapperBenchmark, SetVoxelsY)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());
	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, &voxels.front(), v.region().getHeightInVoxels());
	}
}

BENCHMARK_DEFINE_F(RawVolumeWrapperBenchmark, SetVoxels)(benchmark::State &state) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 22> voxels;
	voxels.assign(voxel, voxels.capacity());

	for (auto _ : state) {
		voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				  v.region().getHeightInVoxels());
	}
}

BENCHMARK_REGISTER_F(RawVolumeWrapperBenchmark, SetVoxel);
BENCHMARK_REGISTER_F(RawVolumeWrapperBenchmark, SetVoxelSampler);
BENCHMARK_REGISTER_F(RawVolumeWrapperBenchmark, SetVoxelsY);
BENCHMARK_REGISTER_F(RawVolumeWrapperBenchmark, SetVoxels);
