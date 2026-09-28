/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ScopedPtr.h"
#include "io/FormatDescription.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/voxelcdx/VCdxFormat.h"
#include "voxelutil/VolumeVisitor.h"
#include "core/collection/DynamicArray.h"

namespace voxelformat {

class VCdxFormatTest : public AbstractFormatTest {};

TEST_F(VCdxFormatTest, testIsA) {
	ASSERT_TRUE(io::isA("Untitled.vcdx", voxelformat::voxelLoad()));
}

TEST_F(VCdxFormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "Untitled.vcdx", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("Model 1", node->name());
	ASSERT_NE(nullptr, node->volume());
	EXPECT_EQ(520, voxelutil::countVoxels(*node->volume()));
}

TEST_F(VCdxFormatTest, testLoadPalette) {
	VCdxFormat f;
	palette::Palette palette;
	ASSERT_GT(helper_loadPalette("Untitled.vcdx", helper_filesystemarchive(), f, palette), 0);
	EXPECT_GE(palette.colorCount(), 170);
	EXPECT_EQ(0, palette.color(0).a);
	EXPECT_GT(palette.color(40).a, 0);
	EXPECT_GT(palette.color(169).a, 0);
}

TEST_F(VCdxFormatTest, testLoadScreenshot) {
	testLoadScreenshot("Untitled.vcdx", 192, 192, color::RGBA(44, 85, 145, 255), 96, 96);
}

TEST_F(VCdxFormatTest, testSaveLoad) {
	VCdxFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All &
									   ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveLoadVoxel("testsaveload.vcdx", &f, 0, 10, flags);
}

TEST_F(VCdxFormatTest, testSaveMultipleModels) {
	VCdxFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All &
									   ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveMultipleModels("testsavemultiple.vcdx", &f, flags);
}

TEST_F(VCdxFormatTest, testSaveLayersWorldSpace) {
	VCdxFormat f;
	palette::Palette pal;
	pal.tryAdd(color::RGBA(255, 0, 0, 255));
	pal.tryAdd(color::RGBA(0, 255, 0, 255));
	const voxel::Region region(0, 0);
	voxel::RawVolume vol1(region);
	voxel::RawVolume vol2(region);
	ASSERT_TRUE(vol1.setVoxel(0, 0, 0, voxel::createVoxel(pal, 0)));
	ASSERT_TRUE(vol2.setVoxel(0, 0, 0, voxel::createVoxel(pal, 1)));

	scenegraph::SceneGraph sceneGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("Red");
		node.setUnownedVolume(&vol1);
		node.setPalette(pal);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("Green");
		node.setUnownedVolume(&vol2);
		node.setPalette(pal);
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(4.0f, 0.0f, 0.0f));
		node.setTransform(0, transform);
		ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(node)));
	}
	sceneGraph.updateTransforms();

	const io::ArchivePtr &archive = helper_archive();
	ASSERT_TRUE(f.save(sceneGraph, "testlayers.vcdx", archive, testSaveCtx));

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(f.load("testlayers.vcdx", archive, loaded, testLoadCtx));
	EXPECT_EQ(2u, loaded.size(scenegraph::SceneGraphNodeType::AllModels));
	EXPECT_EQ(1u, loaded.size(scenegraph::SceneGraphNodeType::Group));

	const scenegraph::SceneGraph::MergeResult &merged = loaded.merge();
	core::ScopedPtr<voxel::RawVolume> volume(merged.volume());
	ASSERT_NE(nullptr, volume);
	EXPECT_EQ(2, voxelutil::countVoxels(*volume));
	EXPECT_FALSE(voxel::isAir(volume->voxel(0, 0, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(volume->voxel(4, 0, 0).getMaterial()));
}

TEST_F(VCdxFormatTest, testCompareMagicaVoxel) {
	scenegraph::SceneGraph vcdxGraph;
	testLoad(vcdxGraph, "Untitled.vcdx", 1);
	scenegraph::SceneGraph voxGraph;
	testLoad(voxGraph, "Untitled.vox", 1);

	const scenegraph::SceneGraphNode *vcdxNode = vcdxGraph.firstModelNode();
	const scenegraph::SceneGraphNode *voxNode = voxGraph.firstModelNode();
	ASSERT_NE(nullptr, vcdxNode);
	ASSERT_NE(nullptr, voxNode);
	ASSERT_NE(nullptr, vcdxNode->volume());
	ASSERT_NE(nullptr, voxNode->volume());

	EXPECT_EQ(voxelutil::countVoxels(*vcdxNode->volume()), voxelutil::countVoxels(*voxNode->volume()));

	core::DynamicArray<color::RGBA> vcdxColors;
	core::DynamicArray<color::RGBA> voxColors;
	voxelutil::visitVolume(*vcdxNode->volume(), [&](int, int, int, const voxel::Voxel &voxel) {
		vcdxColors.push_back(vcdxNode->palette().color(voxel.getColor()));
	});
	voxelutil::visitVolume(*voxNode->volume(), [&](int, int, int, const voxel::Voxel &voxel) {
		voxColors.push_back(voxNode->palette().color(voxel.getColor()));
	});
	ASSERT_EQ(vcdxColors.size(), voxColors.size());
	vcdxColors.sort([](const color::RGBA &a, const color::RGBA &b) {
		return a.rgba < b.rgba;
	});
	voxColors.sort([](const color::RGBA &a, const color::RGBA &b) {
		return a.rgba < b.rgba;
	});
	for (size_t i = 0; i < vcdxColors.size(); ++i) {
		EXPECT_EQ(vcdxColors[i], voxColors[i]) << "color mismatch at sorted voxel " << i;
	}
}

} // namespace voxelformat
