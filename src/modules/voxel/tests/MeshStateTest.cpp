/**
 * @file
 */

#include "voxel/MeshState.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "voxel/MaterialColor.h"
#include "voxel/SurfaceExtractor.h"

namespace voxel {

class MeshStateTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	void SetUp() override {
		Super::SetUp();
		core::Var::registerVar(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::registerVar(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
	}
};

TEST_F(MeshStateTest, testExtractRegion) {
	voxel::RawVolume v(voxel::Region(-1, 1));

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, nullptr, true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	const voxel::Region region(1, 0, 1, 1, 0, 1);
	meshState.scheduleRegionExtraction(0, region);
	EXPECT_EQ(1, meshState.pendingExtractions());

	(void)meshState.shutdown();
}

TEST_F(MeshStateTest, testExtractRegionBoundary) {
	voxel::RawVolume v(voxel::Region(0, 31));

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, nullptr, true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	// worst case scenario - touching all adjacent regions
	const voxel::Region region(15, 15);
	meshState.scheduleRegionExtraction(0, region);
	EXPECT_EQ(8, meshState.pendingExtractions());

	const voxel::Region region2(14, 14);
	meshState.scheduleRegionExtraction(0, region2);
	EXPECT_EQ(9, meshState.pendingExtractions());
	(void)meshState.shutdown();
}

} // namespace voxelrender
