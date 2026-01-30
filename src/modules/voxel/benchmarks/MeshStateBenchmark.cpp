/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/MeshState.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"

class MeshStateBenchmark : public app::AbstractBenchmark {
protected:
	voxel::RawVolume v{voxel::Region{0, 0, 0, 61, 22, 61}};
	voxel::MeshState meshState;

public:
	void onCleanupApp() override {
		(void)meshState.shutdown();
		app::AbstractBenchmark::onCleanupApp();
	}

	bool onInitApp() override {
		if (!app::AbstractBenchmark::onInitApp()) {
			return false;
		}
		core::Var::get(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
		meshState.construct();
		if (!meshState.init()) {
			Log::error("Failed to initialize mesh state");
			return false;
		}
		return true;
	}


	void SetUp(::benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);

		v.setVoxel(6, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 52, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 2));
		v.setVoxel(8, 6, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 53, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 8, 54, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 7, 55, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 5, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(9, 6, 56, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 57, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 58, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 5, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(5, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(6, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(7, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
		v.setVoxel(8, 6, 59, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	}
};

BENCHMARK_DEFINE_F(MeshStateBenchmark, Extract)(benchmark::State &state) {
	palette::Palette palette;
	palette.nippon();
	for (auto _ : state) {
		bool meshDeleted = false;
		(void)meshState.setVolume(0, &v, &palette, nullptr, true, meshDeleted);
		meshState.scheduleRegionExtraction(0, v.region());
		meshState.extractAllPending();
		for (;;) {
			const int idx = meshState.pop();
			if (idx == -1) {
				break;
			}
		}
		meshState.clearMeshes();
		(void)meshState.setVolume(0, nullptr, nullptr, nullptr, true, meshDeleted);
	}
}

BENCHMARK_REGISTER_F(MeshStateBenchmark, Extract);
