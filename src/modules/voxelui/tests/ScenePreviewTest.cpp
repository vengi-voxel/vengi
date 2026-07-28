/**
 * @file
 */

#include "voxelui/ScenePreview.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/tests/AbstractGLTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelui {

class ScenePreviewTest : public video::AbstractGLTest {
private:
	using Super = video::AbstractGLTest;

protected:
	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
	}
};

TEST_F(ScenePreviewTest, testInitShutdown) {
	if (IsSkipped()) {
		return;
	}
	ScenePreview preview(_testApp->timeProvider());
	ASSERT_TRUE(preview.init());
	EXPECT_TRUE(preview.empty());
	preview.shutdown();
}

TEST_F(ScenePreviewTest, testSetSceneGraph) {
	if (IsSkipped()) {
		return;
	}
	ScenePreview preview(_testApp->timeProvider());
	ASSERT_TRUE(preview.init());

	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
	volume->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	palette::Palette pal;
	pal.nippon();
	node.setPalette(pal);
	node.setName("preview");
	ASSERT_GT(sceneGraph.emplace(core::move(node)), 0);

	ASSERT_TRUE(preview.setSceneGraph(core::move(sceneGraph)));
	EXPECT_FALSE(preview.empty());
	preview.clear();
	EXPECT_TRUE(preview.empty());
	preview.shutdown();
}

} // namespace voxelui
