/**
 * @file
 */

#include "voxelui/ScenePreview.h"
#include "color/RGBA.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "image/Image.h"
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

TEST_F(ScenePreviewTest, testMultisampleResolve) {
	if (IsSkipped()) {
		return;
	}
	// Default cl_multisamplesamples is 4; AbstractGLTest forces 0. The file-dialog
	// preview was drawing into the MSAA FBO and sampling the unresolved resolve FBO.
	ASSERT_TRUE(core::getVar(cfg::ClientMultiSampleSamples)->setVal(4));

	ScenePreview preview(_testApp->timeProvider());
	ASSERT_TRUE(preview.init());
	if (!preview.multisamplingEnabled()) {
		GTEST_SKIP() << "MSAA framebuffer not available in this GL context";
	}

	scenegraph::SceneGraph sceneGraph;
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
	for (int x = 0; x < 8; ++x) {
		for (int y = 0; y < 8; ++y) {
			for (int z = 0; z < 8; ++z) {
				volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	palette::Palette pal;
	pal.nippon();
	node.setPalette(pal);
	node.setName("preview");
	ASSERT_GT(sceneGraph.emplace(core::move(node)), 0);

	ASSERT_TRUE(preview.setSceneGraph(core::move(sceneGraph)));
	preview.render(0.016, glm::ivec2(64, 64));

	const image::ImagePtr img = preview.image();
	ASSERT_TRUE(img);
	ASSERT_TRUE(img->isLoaded());
	ASSERT_EQ(64, img->width());
	ASSERT_EQ(64, img->height());

	int visible = 0;
	for (int y = 0; y < img->height(); ++y) {
		for (int x = 0; x < img->width(); ++x) {
			const color::RGBA c = img->colorAt(x, y);
			if ((int)c.r + (int)c.g + (int)c.b > 30) {
				++visible;
			}
		}
	}
	EXPECT_GT(visible, 0) << "Resolve FBO stayed empty; MSAA blit is missing";
	preview.shutdown();
}

} // namespace voxelui
