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
		const core::VarDef voxelMeshSize(cfg::VoxelMeshSize, 16, "", "", core::CV_READONLY);
		core::Var::registerVar(voxelMeshSize);
		const core::VarDef voxRenderMeshMode(cfg::VoxelMeshMode, (int)voxel::SurfaceExtractionType::Binary, "", "");
		core::Var::registerVar(voxRenderMeshMode);
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

TEST_F(MeshStateTest, testSetStateBeforeVolume) {
	// Verify that property setters work for indices beyond current size.
	// This simulates the prepareModelNodes flow where updateNodeState
	// calls hide()/gray()/etc. before setVolume is called.
	MeshState meshState;
	meshState.construct();
	meshState.init();

	const int idx = 5;
	// These must grow the internal arrays and store the state
	meshState.hide(idx, false);
	EXPECT_FALSE(meshState.hidden(idx));

	meshState.gray(idx, true);
	EXPECT_TRUE(meshState.grayed(idx));

	meshState.setLocked(idx, true);
	EXPECT_TRUE(meshState.locked(idx));

	meshState.setHasSelection(idx, true);
	EXPECT_TRUE(meshState.hasSelection(idx));

	// After state setup, setVolume should work normally
	voxel::RawVolume v(voxel::Region(-1, 1));
	palette::Palette pal;
	pal.nippon();
	bool deleted = false;
	(void)meshState.setVolume(idx, &v, &pal, nullptr, true, deleted);
	EXPECT_NE(nullptr, meshState.volume(idx));

	meshState.scheduleRegionExtraction(idx, v.region());
	EXPECT_LT(0, meshState.pendingExtractions());

	(void)meshState.shutdown();
}

} // namespace voxelrender
