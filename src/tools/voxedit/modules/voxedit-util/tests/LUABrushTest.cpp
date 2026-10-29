/**
 * @file
 */

#include "voxedit-util/modifier/brush/LUABrush.h"
#include "app/tests/AbstractTest.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/LUASelectionMode.h"
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
	LUABrush brush(_testApp->filesystem());
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

TEST_F(LUABrushTest, testRetainedVolumeArgumentsAcrossInvocations) {
	const core::String filename = "/tmp/vengi-brush-retained-arguments.lua";
	ASSERT_TRUE(io::Filesystem::sysWrite(filename, core::String("function generate(node, region, color)\n"
																" retained = node:volume()\n"
																" retained:setVoxel(0, 0, 0, color)\n"
																"end\n")));
	LUABrush brush(_testApp->filesystem());
	ASSERT_TRUE(brush.init());
	ASSERT_TRUE(brush.loadScript(filename));
	scenegraph::SceneGraph scene;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(new voxel::RawVolume(voxel::Region(-1, 1)));
	const int id = scene.emplace(core::move(node));
	scene.setActiveNode(id);
	BrushContext input;
	input.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	for (int i = 0; i < 2; ++i) {
		ModifierVolumeWrapper wrapper(scene.node(id), ModifierType::Override);
		ASSERT_TRUE(brush.execute(scene, wrapper, input));
		EXPECT_EQ(2, scene.node(id).volume()->voxel(0, 0, 0).getColor());
	}
	brush.shutdown();
	EXPECT_TRUE(io::Filesystem::sysRemoveFile(filename));
}

TEST_F(LUABrushTest, testRetainedVolumesFinalizeBeforeToolResources) {
	const core::String filename = "/tmp/vengi-lua-tool-finalizers.lua";
	ASSERT_TRUE(io::Filesystem::sysWrite(filename, core::String("function generate(node)\n"
																" retained = node:volume()\n"
																" retained:setVoxel(0, 0, 0, 2)\n"
																"end\n"
																"select = generate\n")));
	scenegraph::SceneGraph scene;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 1)));
	const int id = scene.emplace(core::move(node));
	scene.setActiveNode(id);
	ModifierVolumeWrapper wrapper(scene.node(id), ModifierType::Override);
	BrushContext input;
	// Keep the scene alive while each tool's destructor finalizes retained Lua volumes.
	{
		LUABrush brush(_testApp->filesystem());
		ASSERT_TRUE(brush.init());
		ASSERT_TRUE(brush.loadScript(filename));
		ASSERT_TRUE(brush.execute(scene, wrapper, input));
		EXPECT_EQ(2, scene.node(id).volume()->voxel(0, 0, 0).getColor());
	}
	scene.node(id).volume()->clear();
	{
		LUASelectionMode mode(_testApp->filesystem());
		ASSERT_TRUE(mode.init());
		ASSERT_TRUE(mode.loadScript(filename));
		mode.execute(scene, wrapper, input, wrapper.region(), glm::ivec3(0), voxel::FaceNames::PositiveY);
		EXPECT_EQ(2, scene.node(id).volume()->voxel(0, 0, 0).getColor());
	}
	EXPECT_TRUE(io::Filesystem::sysRemoveFile(filename));
}

} // namespace voxedit
