/**
 * @file
 */

#include "voxelrender/MeshState.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelrender {

class MeshStateTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxelMeshMode, "0");
	}
};

TEST_F(MeshStateTest, testExtractRegion) {
	voxel::RawVolume v(voxel::Region(-1, 1));
	scenegraph::SceneGraphNode node;
	node.setVolume(&v, false);

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	meshState.setVolume(0, node.volume(), &node.palette(), true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	const voxel::Region region(1, 0, 1, 1, 0, 1);
	meshState.extractRegion(0, region);
	EXPECT_EQ(1, meshState.pendingExtractions());

	meshState.shutdown();
}

TEST_F(MeshStateTest, testExtractRegionBoundary) {
	voxel::RawVolume v(voxel::Region(0, 31));
	scenegraph::SceneGraphNode node;
	node.setVolume(&v, false);

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	meshState.setVolume(0, node.volume(), &node.palette(), true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	// worst case scenario - touching all adjacent regions
	const voxel::Region region(15, 15);
	meshState.extractRegion(0, region);
	EXPECT_EQ(8, meshState.pendingExtractions());

	const voxel::Region region2(14, 14);
	meshState.extractRegion(0, region2);
	EXPECT_EQ(9, meshState.pendingExtractions());
	meshState.shutdown();
}

} // namespace voxelrender
