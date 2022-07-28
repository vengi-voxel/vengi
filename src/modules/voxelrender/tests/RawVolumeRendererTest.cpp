/**
 * @file
 */

#include "voxelrender/RawVolumeRenderer.h"
#include "app/tests/AbstractTest.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelrender {

class RawVolumeRendererTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER, "Activate shadow map", core::Var::boolValidator);
		core::Var::get(cfg::ClientBloom, "true", "Activate bloom post processing", core::Var::boolValidator);
	}
};

TEST_F(RawVolumeRendererTest, testExtractRegion) {
	voxel::RawVolume v(voxel::Region(-1, 1));
	voxelformat::SceneGraphNode node;
	node.setVolume(&v, false);

	RawVolumeRenderer renderer;
	renderer.construct();
	renderer.init(glm::ivec2(0));
	renderer.setVolume(0, node);

	EXPECT_EQ(0, renderer.pendingExtractions());
	const voxel::Region region(1, 0, 1, 1, 0, 1);
	renderer.extractRegion(0, region);
	EXPECT_EQ(1, renderer.pendingExtractions());

	renderer.visitPendingExtractions([region](int nodeIdx, const voxel::Region &extractRegion) {
		EXPECT_TRUE(extractRegion.containsRegion(region));
		EXPECT_EQ(glm::ivec3(0), extractRegion.getLowerCorner());
		EXPECT_EQ(glm::ivec3(15), extractRegion.getUpperCorner()); // see cfg::VoxelMeshSize
	});
}

} // namespace voxelrender
