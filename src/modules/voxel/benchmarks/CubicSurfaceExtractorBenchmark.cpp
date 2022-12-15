/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"

static constexpr int MAX_BENCHMARK_VOLUME_SIZE = 64;
static const int meshSize = 128;
class CubicSurfaceExtractorBenchmark : public app::AbstractBenchmark {
public:
	void onCleanupApp() override {
	}

	template<class Volume>
	void fill(const voxel::Region& region, Volume* v) const {
		const voxel::Voxel voxel = voxel::createVoxel(1);
		for (int x = region.getLowerX(); x < region.getUpperX(); ++x) {
			for (int y = region.getLowerY(); y < region.getUpperY(); ++y) {
				for (int z = region.getLowerZ(); z < region.getUpperZ(); ++z) {
					if (x + y + z % 2 == 0) {
						v->setVoxel(x, y, z, voxel);
					}
				}
			}
		}
	}
};

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedy)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtract)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	fill(region, &volume);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedyEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), true, true);
	}
}

BENCHMARK_DEFINE_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractEmpty)(benchmark::State &state) {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(state.range(0), meshSize, state.range(0)));
	const voxel::Region volumeRegion(0, MAX_BENCHMARK_VOLUME_SIZE);
	voxel::RawVolume volume(volumeRegion);
	voxel::Mesh mesh(1024 * 1024, 1024 * 1024, false);
	for (auto _ : state) {
		voxel::extractCubicMesh(&volume, region, &mesh, voxel::IsQuadNeeded(), region.getLowerCorner(), false, false);
	}
}

BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedy)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtract)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractGreedyEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);
BENCHMARK_REGISTER_F(CubicSurfaceExtractorBenchmark, RawVolumeExtractEmpty)->RangeMultiplier(2)->Range(16, MAX_BENCHMARK_VOLUME_SIZE);

BENCHMARK_MAIN();
