/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerUtil.h"

class VolumeSamplerPositionBenchmark : public app::AbstractBenchmark {
protected:
	bool populate(voxel::RawVolume &volume) {
		for (int z = -40; z < 40; ++z) {
			for (int y = -40; y < 40; ++y) {
				for (int x = -40; x < 40; ++x) {
					if (!volume.setVoxel(x, y, z, voxel::Voxel((x + y + z) % 5 == 0 ? voxel::VoxelType::Air : voxel::VoxelType::Generic,
						(uint8_t)(x + 3 * y + 7 * z), 9, voxel::FlagOutline, 7))) {
						return false;
					}
				}
			}
		}
		return true;
	}

	glm::ivec3 nextPosition(uint32_t &seed) {
		seed = seed * 1664525u + 1013904223u;
		return glm::ivec3(seed % 80, (seed >> 8) % 80, (seed >> 16) % 80) - 40;
	}
};

BENCHMARK_DEFINE_F(VolumeSamplerPositionBenchmark, SetPositionChanging)(benchmark::State &state) {
	const int previous = voxel::RawVolume::chunkedStorageThreshold();
	voxel::RawVolume::setChunkedStorageThreshold(state.range(0) ? 1 : 256);
	voxel::RawVolume volume(voxel::Region(-40, 39));
	voxel::RawVolume::setChunkedStorageThreshold(previous);
	if (!volume.valid()) {
		state.SkipWithError("Could not allocate comparison volume");
		return;
	}
	if (!populate(volume)) {
		state.SkipWithError("Could not populate comparison volume");
		return;
	}
	voxel::RawVolume::Sampler sampler(volume);
	uint32_t seed = 42;
	for (auto _ : state) {
		glm::ivec3 pos = nextPosition(seed);
		benchmark::DoNotOptimize(pos);
		sampler.setPosition(pos);
		voxel::Voxel value = sampler.voxel();
		benchmark::DoNotOptimize(value);
	}
}

BENCHMARK_DEFINE_F(VolumeSamplerPositionBenchmark, SampleLinearChanging)(benchmark::State &state) {
	const int previous = voxel::RawVolume::chunkedStorageThreshold();
	voxel::RawVolume::setChunkedStorageThreshold(state.range(0) ? 1 : 256);
	voxel::RawVolume volume(voxel::Region(-40, 39));
	voxel::RawVolume::setChunkedStorageThreshold(previous);
	if (!volume.valid()) {
		state.SkipWithError("Could not allocate comparison volume");
		return;
	}
	if (!populate(volume)) {
		state.SkipWithError("Could not populate comparison volume");
		return;
	}
	voxel::RawVolume::Sampler sampler(volume);
	uint32_t seed = 42;
	for (auto _ : state) {
		const glm::ivec3 cell = nextPosition(seed);
		const glm::vec3 fraction((seed >> 26) & 3u, (seed >> 28) & 3u, (seed >> 30) & 3u);
		glm::vec3 pos = glm::vec3(cell) + fraction * 0.25f;
		benchmark::DoNotOptimize(pos);
		voxel::Voxel value = voxel::sampleTrilinear(sampler, pos);
		benchmark::DoNotOptimize(value);
	}
}

// The same coordinates and populated data exercise dense (0) and chunked (1) storage.
BENCHMARK_REGISTER_F(VolumeSamplerPositionBenchmark, SetPositionChanging)->Arg(0)->Arg(1);
BENCHMARK_REGISTER_F(VolumeSamplerPositionBenchmark, SampleLinearChanging)->Arg(0)->Arg(1);
