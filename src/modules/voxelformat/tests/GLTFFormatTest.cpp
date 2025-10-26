/**
 * @file
 */

#include "voxelformat/private/mesh/GLTFFormat.h"
#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class GLTFFormatTest : public AbstractFormatTest {};

TEST_F(GLTFFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "rgb.qb");
	helper_saveSceneGraph(sceneGraph, "exportrgb.gltf");
}

TEST_F(GLTFFormatTest, testImportAnimation) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/BoxAnimated.glb", 2);
	auto iter = sceneGraph.beginModel();
	++iter;
	scenegraph::SceneGraphNode &node = *iter;
	EXPECT_GE(sceneGraph.animations().size(), 1u);
	EXPECT_EQ("animation 0", sceneGraph.animations().back());
	EXPECT_TRUE(sceneGraph.setAnimation(sceneGraph.animations().back()));
	ASSERT_FALSE(node.keyFrames()->empty());
	ASSERT_GE(node.keyFrames()->size(), 2u);
}

TEST_F(GLTFFormatTest, testVoxelizeCube) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/cube/Cube.gltf", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(nullptr, v);
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1, -1, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1, 0, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(0, -1, -1).getMaterial()));
}

TEST_F(GLTFFormatTest, testRGB) {
	testRGB("rgb.gltf");
}

TEST_F(GLTFFormatTest, testSaveLoadVoxel) {
	GLTFFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("bv-smallvolumesavetest.gltf", &f, 0, 10, flags);
}

// TODO: MATERIAL: materials are not yet properly loaded back from gltf
TEST_F(GLTFFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.gltf");
}

class VoxelizeLantern : public AbstractFormatTest, public ::testing::WithParamInterface<bool> {};

TEST_P(VoxelizeLantern, exec) {
	bool params = GetParam();
	util::ScopedVarChange var(cfg::VoxelCreatePalette, params);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/lantern/Lantern.gltf", 3u);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("LanternPole_Body", node->name());
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(nullptr, v);
	const voxel::Region &region = v->region();
	EXPECT_EQ(-9, region.getLowerX());
	EXPECT_EQ(-14, region.getLowerY());
	EXPECT_EQ(-4, region.getLowerZ());
	EXPECT_EQ(8, region.getUpperX());
	EXPECT_EQ(13, region.getUpperY());
	EXPECT_EQ(3, region.getUpperZ());
	EXPECT_EQ(286, voxelutil::countVoxels(*v));
	// TODO: VOXELFORMAT: https://github.com/vengi-voxel/vengi/issues/620
	// EXPECT_EQ(89, v->voxel(-8, 9, 0).getColor());
	const core::RGBA expected(69, 58, 46, 255);
	const core::RGBA is = node->palette().color(v->voxel(-8, 9, 0).getColor());
	voxel::colorComparatorDistance(expected, is, 0.01f);
}

INSTANTIATE_TEST_SUITE_P(
	GLTFFormatTest,
	VoxelizeLantern,
	::testing::Values(true, false),
	[](const testing::TestParamInfo<bool>& nfo) {
		if (nfo.param) {
			return "createpalette";
		}
		return "nocreatepalette";
	}
);

} // namespace voxelformat
