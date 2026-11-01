/**
 * @file
 */

#include "scenegraph/SceneGraphImage.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace scenegraph {

class SceneGraphImageTest : public app::AbstractTest {};

TEST_F(SceneGraphImageTest, testRenderToImageLimitsToNodeAndChildren) {
	voxel::RawVolume parentVol(voxel::Region(0, 0));
	parentVol.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume childVol(voxel::Region(glm::ivec3(2, 0, 0), glm::ivec3(2, 0, 0)));
	childVol.setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume siblingVol(voxel::Region(glm::ivec3(10, 0, 0), glm::ivec3(10, 0, 0)));
	siblingVol.setVoxel(10, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	palette::Palette parentPalette;
	parentPalette.setColor(1, color::RGBA(255, 0, 0, 255));
	palette::Palette childPalette;
	childPalette.setColor(1, color::RGBA(0, 255, 0, 255));
	palette::Palette siblingPalette;
	siblingPalette.setColor(1, color::RGBA(0, 0, 255, 255));

	SceneGraph sceneGraph;
	int parentId = InvalidNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("parent");
		node.setUnownedVolume(&parentVol);
		node.setPalette(parentPalette);
		parentId = sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("child");
		node.setUnownedVolume(&childVol);
		node.setPalette(childPalette);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node), parentId));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("sibling");
		node.setUnownedVolume(&siblingVol);
		node.setPalette(siblingPalette);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}

	voxelutil::ImageRenderConfig config;
	config.background = color::RGBA(0, 0, 0, 0);
	config.upScale = false;

	const image::ImagePtr scene = voxelutil::renderToImage(sceneGraph, config);
	ASSERT_TRUE(scene && scene->isLoaded());
	EXPECT_EQ(11, scene->width());
	EXPECT_EQ(1, scene->height());
	EXPECT_EQ(scene->colorAt(10, 0).rgba, color::RGBA(255, 0, 0, 255).rgba);
	EXPECT_EQ(scene->colorAt(8, 0).rgba, color::RGBA(0, 255, 0, 255).rgba);
	EXPECT_EQ(scene->colorAt(0, 0).rgba, color::RGBA(0, 0, 255, 255).rgba);

	const image::ImagePtr subtree = voxelutil::renderToImage(sceneGraph, config, parentId);
	ASSERT_TRUE(subtree && subtree->isLoaded());
	EXPECT_EQ(3, subtree->width());
	EXPECT_EQ(subtree->colorAt(2, 0).rgba, color::RGBA(255, 0, 0, 255).rgba);
	EXPECT_EQ(subtree->colorAt(0, 0).rgba, color::RGBA(0, 255, 0, 255).rgba);
	EXPECT_EQ(subtree->colorAt(1, 0).rgba, config.background.rgba);

	EXPECT_FALSE(voxelutil::renderToImage(sceneGraph, config, 99));
}

TEST_F(SceneGraphImageTest, testRenderToImageKeepsPaletteAlpha) {
	voxel::RawVolume solid(voxel::Region(0, 0));
	solid.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume glass(voxel::Region(glm::ivec3(2, 0, 0), glm::ivec3(2, 0, 0)));
	glass.setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));

	palette::Palette opaque;
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		opaque.setColor((uint8_t)i, color::RGBA((uint8_t)i, 0, 0, 255));
	}
	palette::Palette clear = opaque;
	clear.setColor(2, color::RGBA(255, 255, 255, 40));

	SceneGraph sceneGraph;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("solid");
		node.setUnownedVolume(&solid);
		node.setPalette(opaque);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("glass");
		node.setUnownedVolume(&glass);
		node.setPalette(clear);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}

	voxelutil::ImageRenderConfig config;
	config.background = color::RGBA(0, 0, 0, 0);
	config.upScale = false;
	const image::ImagePtr image = voxelutil::renderToImage(sceneGraph, config);
	ASSERT_TRUE(image && image->isLoaded());
	EXPECT_EQ(image->colorAt(0, 0).rgba, color::RGBA(255, 255, 255, 40).rgba);
}

TEST_F(SceneGraphImageTest, testRenderIsometricImageSubtreeIsSmaller) {
	voxel::RawVolume parentVol(voxel::Region(0, 0));
	parentVol.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume childVol(voxel::Region(glm::ivec3(2, 0, 0), glm::ivec3(2, 0, 0)));
	childVol.setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume siblingVol(voxel::Region(glm::ivec3(10, 0, 0), glm::ivec3(10, 0, 0)));
	siblingVol.setVoxel(10, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	palette::Palette palette;
	palette.setColor(1, color::RGBA(200, 200, 200, 255));

	SceneGraph sceneGraph;
	int parentId = InvalidNodeId;
	int siblingId = InvalidNodeId;
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("parent");
		node.setUnownedVolume(&parentVol);
		node.setPalette(palette);
		parentId = sceneGraph.emplace(core::move(node));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("child");
		node.setUnownedVolume(&childVol);
		node.setPalette(palette);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node), parentId));
	}
	{
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setName("sibling");
		node.setUnownedVolume(&siblingVol);
		node.setPalette(palette);
		siblingId = sceneGraph.emplace(core::move(node));
		ASSERT_NE(InvalidNodeId, siblingId);
	}

	voxelutil::IsoRenderConfig config;
	config.shading = false;
	config.materials = false;
	config.transparency = false;
	const image::ImagePtr scene = voxelutil::renderIsometricImage(sceneGraph, config);
	const image::ImagePtr subtree = voxelutil::renderIsometricImage(sceneGraph, config, parentId);
	const image::ImagePtr sibling = voxelutil::renderIsometricImage(sceneGraph, config, siblingId);
	ASSERT_TRUE(scene && subtree && sibling);
	EXPECT_EQ(8, sibling->width());
	EXPECT_EQ(7, sibling->height());
	EXPECT_GT(scene->width(), subtree->width());
	EXPECT_GT(subtree->width(), sibling->width());
}

} // namespace scenegraph
