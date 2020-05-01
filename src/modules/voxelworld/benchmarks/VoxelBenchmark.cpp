/**
 * @file
 */

#include "core/benchmark/AbstractBenchmark.h"
#include "voxelworld/WorldPager.h"
#include "voxel/PagedVolume.h"
#include "voxelworld/BiomeManager.h"
#include "voxel/Constants.h"
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
	voxelworld::WorldPager pager(_volumeCache, std::make_shared<voxelworld::ChunkPersister>());
	pager.setSeed(0l);
	int chunkSize = 256;
	voxel::PagedVolume volumeData(&pager, 1024 * 1024 * 1024, chunkSize);
	const io::FilesystemPtr& filesystem = io::filesystem();
	const core::String& luaParameters = filesystem->load("worldparams.lua");
	const core::String& luaBiomes = filesystem->load("biomes.lua");
	pager.init(&volumeData, luaParameters, luaBiomes);
	int i = 0;
	while (state.KeepRunning()) {
		volumeData.voxel(chunkSize * i, 0, 0);
		++i;
	}
}

BENCHMARK_REGISTER_F(PagedVolumeBenchmark, pageIn);

BENCHMARK_MAIN();
