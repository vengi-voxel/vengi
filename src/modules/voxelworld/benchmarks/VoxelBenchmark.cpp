#include "core/benchmark/AbstractBenchmark.h"
#include "voxelworld/WorldPager.h"
#include "voxel/PagedVolume.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxelworld/BiomeManager.h"
#include "voxel/Constants.h"
#include "voxel/IsQuadNeeded.h"
#include "voxelformat/VolumeCache.h"

class PagedVolumeBenchmark: public core::AbstractBenchmark {
protected:
	voxelformat::VolumeCachePtr _volumeCache;

public:
	void onCleanupApp() override {
		if (_volumeCache) {
			_volumeCache->shutdown();
		}
	}

	bool onInitApp() override {
		voxel::initDefaultMaterialColors();
		_volumeCache = std::make_shared<voxelformat::VolumeCache>();
		return _volumeCache->init();
	}
};

BENCHMARK_DEFINE_F(PagedVolumeBenchmark, pageIn) (benchmark::State& state) {
	const uint16_t chunkSideLength = state.range(0);
	const uint32_t volumeMemoryMegaBytes = chunkSideLength * 2;
	voxelworld::WorldPager pager(_volumeCache, std::make_shared<voxelworld::ChunkPersister>());
	pager.setSeed(0l);
	voxel::PagedVolume *volumeData = new voxel::PagedVolume(&pager, volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);
	const io::FilesystemPtr& filesystem = io::filesystem();
	const core::String& luaParameters = filesystem->load("worldparams.lua");
	const core::String& luaBiomes = filesystem->load("biomes.lua");
	pager.init(volumeData, luaParameters, luaBiomes);
	const glm::ivec3 meshSize(16, 128, 16);
	int x = 0;
	while (state.KeepRunning()) {
		glm::ivec3 mins(x, 0, 0);
		x += meshSize.x;
		voxel::Region region(mins, mins + meshSize);
		voxel::Mesh mesh(0, 0, true);
		voxel::extractCubicMesh(volumeData, region, &mesh, voxel::IsQuadNeeded());
	}
	delete volumeData;
}

BENCHMARK_REGISTER_F(PagedVolumeBenchmark, pageIn)->RangeMultiplier(2)->Range(8, 256);

BENCHMARK_MAIN();
