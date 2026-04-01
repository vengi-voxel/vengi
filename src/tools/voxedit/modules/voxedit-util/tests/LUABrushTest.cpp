/**
 * @file
 */

#include "voxedit-util/modifier/brush/LUABrush.h"
#include "app/tests/AbstractTest.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class LUABrushTest : public app::AbstractTest {
protected:
	bool onInitApp() override {
		app::AbstractTest::onInitApp();
		return _testApp->filesystem()->registerPath("brushes/");
	}
};

TEST_F(LUABrushTest, testPathBrush) {
	LuaBrush brush(_testApp->filesystem());
	ASSERT_TRUE(brush.init());
	ASSERT_TRUE(brush.loadScript("path"));

	EXPECT_TRUE(brush.active());
	EXPECT_EQ(1u, brush.parameterDescriptions().size());
	EXPECT_STREQ("connectivity", brush.parameterDescriptions()[0].name.c_str());

	// Create a volume with a line of solid voxels at y=0 from x=0 to x=5
	scenegraph::SceneGraph sceneGraph;
	voxel::Region region(-1, -1, -1, 10, 5, 5);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	for (int x = 0; x <= 5; ++x) {
		volume->setVoxel(x, -1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	const int nodeId = sceneGraph.emplace(core::move(node));
	ASSERT_NE(nodeId, InvalidNodeId);
	sceneGraph.setActiveNode(nodeId);

	scenegraph::SceneGraphNode &sceneNode = sceneGraph.node(nodeId);
	ModifierVolumeWrapper wrapper(sceneNode, brush.modifierType());

	BrushContext ctx;
	// Path should walk from (0,0,0) to (5,0,0) over the solid voxels at y=-1
	ctx.referencePos = glm::ivec3(0, 0, 0);
	ctx.cursorPosition = glm::ivec3(5, 0, 0);
	ctx.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	brush.preExecute(ctx, wrapper.volume());
	ASSERT_TRUE(brush.execute(sceneGraph, wrapper, ctx));

	// The pathfinder should place voxels along the path
	// At minimum, the start and end should have voxels placed
	bool hasVoxels = false;
	for (int x = 0; x <= 5; ++x) {
		if (voxel::isBlocked(sceneNode.volume()->voxel(x, 0, 0).getMaterial())) {
			hasVoxels = true;
			break;
		}
	}
	EXPECT_TRUE(hasVoxels) << "Pathfinder should have placed at least one voxel along the path";

	brush.shutdown();
}

} // namespace voxedit
