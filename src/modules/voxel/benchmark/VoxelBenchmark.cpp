#include "core/benchmark/AbstractBenchmark.h"
#include "voxel/WorldPager.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/WorldContext.h"
#include "voxel/BiomeManager.h"

class PagedVolumeBenchmark: public core::AbstractBenchmark {
private:
	voxel::WorldPager _pager;
	voxel::PagedVolume *_volumeData = nullptr;
	voxel::BiomeManager _biomeManager;
	voxel::WorldContext _ctx;

public:
	void onCleanupApp() override {
		delete _volumeData;
		_volumeData = nullptr;
		_biomeManager.shutdown();
	}

	bool onInitApp() override {
		voxel::initDefaultMaterialColors();
		const std::string& luaParameters = core::App::getInstance()->filesystem()->load("world.lua");
		const std::string& luaBiomes = core::App::getInstance()->filesystem()->load("biomes.lua");
		Log::info("%s", luaParameters.c_str());
		Log::info("%s", luaBiomes.c_str());
		_biomeManager.init(luaBiomes);
		_ctx.load(luaParameters);
		const uint32_t volumeMemoryMegaBytes = 512;
		const uint16_t chunkSideLength = 256;
		_volumeData = new voxel::PagedVolume(&_pager, volumeMemoryMegaBytes * 1024 * 1024, chunkSideLength);
		_pager.init(_volumeData, &_biomeManager, &_ctx);
		return true;
	}
};

BENCHMARK_F(PagedVolumeBenchmark, pageIn) (benchmark::State& st) {
	while (st.KeepRunning()) {
	}
}

BENCHMARK_REGISTER_F(PagedVolumeBenchmark, pageIn)->Threads(2);

BENCHMARK_MAIN()
