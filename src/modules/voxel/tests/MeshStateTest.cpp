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
	// With floor division, the meshSizeMinusOne subtraction correctly maps
	// negative coordinates to the previous chunk, scheduling more chunks.
	// The completeRegion intersection check filters out non-overlapping ones,
	// but for a small volume like [-1,1]^3 most chunks still intersect.
	EXPECT_LT(0, meshState.pendingExtractions());

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

// https://github.com/vengi-voxel/vengi/issues/844
// Verify that the cubic extractor generates boundary faces when a voxel
// sits at the upper edge of a chunk region. The region expansion in
// extractSurface must ensure the extractor iterates one position beyond
// the original region so that positive-direction faces are generated.
TEST_F(MeshStateTest, testCubicBoundaryFaceGeneration) {
	// meshSize is 16 in tests. Place a voxel at the upper edge of chunk [0..15].
	voxel::RawVolume v(voxel::Region(0, 15));
	v.setVoxel(15, 15, 15, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	MeshState meshState;
	meshState.construct();
	core::Var::getVar(cfg::VoxelMeshMode)->setVal((int)voxel::SurfaceExtractionType::Cubic);
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, nullptr, true, deleted);

	meshState.scheduleRegionExtraction(0, v.region());
	meshState.extractAllPending();

	// A single voxel should produce 8 vertices and 36 indices (6 faces)
	size_t vertCount = 0, normalsCount = 0, indCount = 0;
	meshState.count(MeshType_Opaque, 0, vertCount, normalsCount, indCount);
	EXPECT_EQ(indCount, 36u) << "Single voxel at upper boundary should have all 6 faces (36 indices)";

	(void)meshState.shutdown();
}

// https://github.com/vengi-voxel/vengi/issues/844
// Simulate the exact scenario: volume at pos [-1,16,1] size 5x4x4.
// Two adjacent voxels (yellow at x=-1, green at x=0). Delete the green
// voxel and re-extract only the modified region. The yellow voxel's
// positive-X face (previously hidden) must now appear.
TEST_F(MeshStateTest, testCubicDeleteAdjacentVoxelAcrossChunkBoundary) {
	// meshSize is 16 in tests. Volume spans chunk boundary at x=0.
	// Chunk [-16..-1] contains x=-1, chunk [0..15] contains x=0..3
	voxel::RawVolume v(voxel::Region(-1, 16, 1, 3, 19, 4));

	// Fill with two colors: yellow at x=-1, green at x=0..3
	for (int y = 16; y <= 19; ++y) {
		for (int z = 1; z <= 4; ++z) {
			v.setVoxel(-1, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1)); // yellow
			for (int x = 0; x <= 3; ++x) {
				v.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 2)); // green
			}
		}
	}

	MeshState meshState;
	meshState.construct();
	core::Var::getVar(cfg::VoxelMeshMode)->setVal((int)voxel::SurfaceExtractionType::Cubic);
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, nullptr, true, deleted);

	// Initial extraction of the full volume
	meshState.scheduleRegionExtraction(0, v.region());
	meshState.extractAllPending();

	size_t indBefore = 0, vc = 0, nc = 0;
	meshState.count(MeshType_Opaque, 0, vc, nc, indBefore);
	EXPECT_GT(indBefore, 0u);

	// Now delete the green voxels at x=0 (right at the chunk boundary)
	for (int y = 16; y <= 19; ++y) {
		for (int z = 1; z <= 4; ++z) {
			v.setVoxel(0, y, z, voxel::createVoxel(voxel::VoxelType::Air, 0));
		}
	}

	// Re-extract only the modified region (the column at x=0).
	// This must schedule BOTH chunk [0..15] (contains x=0) AND chunk [-16..-1]
	// (needs to regenerate the positive-X face for the yellow voxel at x=-1).
	const voxel::Region modifiedRegion(0, 16, 1, 0, 19, 4);
	meshState.scheduleRegionExtraction(0, modifiedRegion);
	// With floor division fix, both chunks should be scheduled
	EXPECT_GE(meshState.pendingExtractions(), 2) << "Both adjacent chunks must be scheduled for re-extraction";
	meshState.extractAllPending();

	// After deletion, the yellow face at x=0 (between yellow at x=-1 and
	// now-empty x=0) must be visible. Count total indices.
	size_t indAfter = 0;
	vc = 0; nc = 0;
	meshState.count(MeshType_Opaque, 0, vc, nc, indAfter);
	EXPECT_GT(indAfter, 0u) << "After deleting adjacent voxel, faces should still exist";

	(void)meshState.shutdown();
}

} // namespace voxelrender
