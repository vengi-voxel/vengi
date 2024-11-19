/**
 * @file
 */

#include "voxelformat/private/mesh/GLTFFormat.h"
#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Voxel.h"

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

TEST_F(GLTFFormatTest, testVoxelizeLantern) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/lantern/Lantern.gltf", 3u);
}

// TODO: MATERIAL: materials are not yet properly loaded back from gltf
TEST_F(GLTFFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.gltf");
}

} // namespace voxelformat
