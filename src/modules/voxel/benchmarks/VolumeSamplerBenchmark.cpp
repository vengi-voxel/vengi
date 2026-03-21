/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxel/VoxelSampling.h"

class VolumeSamplerBenchmark : public app::AbstractBenchmark {
protected:
	static constexpr int Size = 31;
	voxel::RawVolume v{voxel::Region{0, 0, 0, Size, Size, Size}};

	void SetUp(benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		// Fill volume with non-air voxels so sampling has data to work with
		for (int z = 0; z <= Size; ++z) {
			for (int y = 0; y <= Size; ++y) {
				for (int x = 0; x <= Size; ++x) {
					v.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, (uint8_t)((x + y + z) % 255 + 1)));
				}
			}
		}
	}
};

BENCHMARK_DEFINE_F(VolumeSamplerBenchmark, SampleNearest)(benchmark::State &state) {
	voxel::RawVolume::Sampler sampler(&v);
	const glm::vec3 pos(15.3f, 15.7f, 15.1f);
	for (auto _ : state) {
		benchmark::DoNotOptimize(voxel::sampleVoxel(sampler, voxel::VoxelSampling::Nearest, pos));
	}
}

BENCHMARK_DEFINE_F(VolumeSamplerBenchmark, SampleLinear)(benchmark::State &state) {
	voxel::RawVolume::Sampler sampler(&v);
	const glm::vec3 pos(15.3f, 15.7f, 15.1f);
	for (auto _ : state) {
		benchmark::DoNotOptimize(voxel::sampleVoxel(sampler, voxel::VoxelSampling::Linear, pos));
	}
}

BENCHMARK_DEFINE_F(VolumeSamplerBenchmark, SampleCubic)(benchmark::State &state) {
	voxel::RawVolume::Sampler sampler(&v);
	const glm::vec3 pos(15.3f, 15.7f, 15.1f);
	for (auto _ : state) {
		benchmark::DoNotOptimize(voxel::sampleVoxel(sampler, voxel::VoxelSampling::Cubic, pos));
	}
}

BENCHMARK_REGISTER_F(VolumeSamplerBenchmark, SampleNearest);
BENCHMARK_REGISTER_F(VolumeSamplerBenchmark, SampleLinear);
BENCHMARK_REGISTER_F(VolumeSamplerBenchmark, SampleCubic);
