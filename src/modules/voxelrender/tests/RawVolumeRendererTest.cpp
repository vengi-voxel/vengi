/**
 * @file
 */

#include "voxelrender/RawVolumeRenderer.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelrender {

class RawVolumeRendererTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxelMeshMode, "0");
		core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER, "Activate shadow map", core::Var::boolValidator);
		core::Var::get(cfg::ClientBloom, "true", "Activate bloom post processing", core::Var::boolValidator);
	}
};

TEST_F(RawVolumeRendererTest, testExtractRegion) {
	voxel::RawVolume v(voxel::Region(-1, 1));
	scenegraph::SceneGraphNode node;
	node.setVolume(&v, false);

	RawVolumeRenderer renderer;
	renderer.construct();
	renderer.init();
	renderer.setVolume(0, node);

	RenderContext rendererContext;
	EXPECT_EQ(0, renderer.pendingExtractions());
	const voxel::Region region(1, 0, 1, 1, 0, 1);
	renderer.extractRegion(0, region);
	EXPECT_EQ(1, renderer.pendingExtractions());

	renderer.shutdown();
}

TEST_F(RawVolumeRendererTest, testExtractRegionBoundary) {
	voxel::RawVolume v(voxel::Region(0, 31));
	scenegraph::SceneGraphNode node;
	node.setVolume(&v, false);

	RawVolumeRenderer renderer;
	renderer.construct();
	renderer.init();
	renderer.setVolume(0, node);

	RenderContext rendererContext;
	EXPECT_EQ(0, renderer.pendingExtractions());
	// worst case scenario - touching all adjacent regions
	const voxel::Region region(15, 15);
	renderer.extractRegion(0, region);
	EXPECT_EQ(8, renderer.pendingExtractions());

	const voxel::Region region2(14, 14);
	renderer.extractRegion(0, region2);
	EXPECT_EQ(9, renderer.pendingExtractions());
	renderer.shutdown();
}

} // namespace voxelrender
