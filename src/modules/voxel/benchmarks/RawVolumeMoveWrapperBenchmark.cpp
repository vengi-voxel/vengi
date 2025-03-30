/**
 * @file
 */

 #include "app/benchmark/AbstractBenchmark.h"
 #include "voxel/RawVolume.h"
 #include "voxel/RawVolumeMoveWrapper.h"
 #include "voxel/SurfaceExtractor.h"

 class RawVolumeMoveWrapperBenchmark : public app::AbstractBenchmark {
 protected:
	 voxel::RawVolume v{voxel::Region{0, 0, 0, 143, 22, 134}};
 };

 BENCHMARK_DEFINE_F(RawVolumeMoveWrapperBenchmark, SetVoxel)(benchmark::State &state) {
	 for (auto _ : state) {
		 voxel::RawVolumeMoveWrapper wrapper(&v);
		 wrapper.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		 wrapper.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	 }
 }

 BENCHMARK_DEFINE_F(RawVolumeMoveWrapperBenchmark, SetVoxelSampler)(benchmark::State &state) {
	 voxel::RawVolumeMoveWrapper wrapper(&v);
	 voxel::RawVolumeMoveWrapper::Sampler sampler(&wrapper);
	 for (auto _ : state) {
		 sampler.setPosition(96, 6, 62);
		 sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
		 sampler.movePositiveY();
		 sampler.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 47));
	 }
 }

 BENCHMARK_REGISTER_F(RawVolumeMoveWrapperBenchmark, SetVoxel);
 BENCHMARK_REGISTER_F(RawVolumeMoveWrapperBenchmark, SetVoxelSampler);
