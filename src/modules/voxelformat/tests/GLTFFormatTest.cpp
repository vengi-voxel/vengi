/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "io/FileStream.h"

namespace voxelformat {

class GLTFFormatTest : public AbstractVoxFormatTest {};

TEST_F(GLTFFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	{
		QBFormat sourceFormat;
		const core::String filename = "rgb.qb";
		const io::FilePtr &file = open(filename);
		io::FileStream stream(file);
		EXPECT_TRUE(sourceFormat.load(filename, stream, sceneGraph, testLoadCtx));
	}
	ASSERT_TRUE(sceneGraph.size() > 0);
	GLTFFormat f;
	const core::String outFilename = "exportrgb.gltf";
	const io::FilePtr &outFile = open(outFilename, io::FileMode::SysWrite);
	io::FileStream outStream(outFile);
	EXPECT_TRUE(f.saveGroups(sceneGraph, outFilename, outStream, testSaveCtx));
}

TEST_F(GLTFFormatTest, testImportAnimation) {
	GLTFFormat f;
	const core::String filename = "glTF/BoxAnimated.glb";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	ASSERT_EQ(2u, sceneGraph.size());
	auto iter = sceneGraph.beginModel();
	++iter;
	scenegraph::SceneGraphNode& node = *iter;
	EXPECT_GE(sceneGraph.animations().size(), 1u);
	EXPECT_EQ("animation 0", sceneGraph.animations().back());
	EXPECT_TRUE(sceneGraph.setAnimation(sceneGraph.animations().back()));
	ASSERT_FALSE(node.keyFrames()->empty());
	ASSERT_GE(node.keyFrames()->size(), 2u);
}

TEST_F(GLTFFormatTest, testVoxelizeCube) {
	GLTFFormat f;
	const core::String filename = "glTF/cube/Cube.gltf";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	ASSERT_EQ(1u, sceneGraph.size());
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(nullptr, v);
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1, -1, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1,  0, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel( 0,  0,  0).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel( 0, -1, -1).getMaterial()));
}

TEST_F(GLTFFormatTest, testRGB) {
	testRGB("rgb.gltf");
}

TEST_F(GLTFFormatTest, testSaveLoadVoxel) {
	GLTFFormat f;
	testSaveLoadVoxel("bv-smallvolumesavetest.gltf", &f, 0, 10);
}

TEST_F(GLTFFormatTest, testVoxelizeLantern) {
	GLTFFormat f;
	const core::String filename = "glTF/lantern/Lantern.gltf";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(GLTFFormatTest, DISABLED_testMaterials) {
	// load the mv scenegraph
	VoxFormat mv;
	scenegraph::SceneGraph mvSceneGraph;
	ASSERT_TRUE(loadGroups("test_material.vox", mv, mvSceneGraph));
	ASSERT_EQ(12u, mvSceneGraph.size());
	// now save as gltf
	ASSERT_TRUE(saveSceneGraph(mvSceneGraph, "test_material.gltf"));
	// load the gltf scenegraph
	GLTFFormat gltf;
	scenegraph::SceneGraph gltfSceneGraph;
	ASSERT_TRUE(loadGroups("test_material.gltf", gltf, gltfSceneGraph));
	ASSERT_EQ(12u, gltfSceneGraph.size());
	// compare the materials
	for (auto iter = mvSceneGraph.beginModel(), iter2 = gltfSceneGraph.beginModel(); iter != mvSceneGraph.end(); ++iter, ++iter2) {
		const scenegraph::SceneGraphNode &mvNode = *iter;
		const scenegraph::SceneGraphNode &gltfNode = *iter2;
		const palette::Palette &mvPalette = mvNode.palette();
		const palette::Palette &gltfPalette = gltfNode.palette();
		for (int i = 0; i < gltfPalette.colorCount(); ++i) {
			int foundColorMatch = -1;
			int foundMaterialMatch = -1;
			const palette::Material &gltfMaterial = gltfPalette.material(i);
			for (int j = 0; j < mvPalette.colorCount(); ++j) {
				// check if the color matches the gltf palette color
				if (gltfPalette.color(i) != mvPalette.color(j)) {
					continue;
				}
				foundColorMatch = true;
				const palette::Material &mvMaterial = mvPalette.material(j);
				if (mvMaterial == gltfMaterial) {
					foundMaterialMatch = j;
					break;
				}
			}
			ASSERT_NE(-1, foundColorMatch) << "Could not find a color match in the magicavoxel palette";
			ASSERT_NE(-1, foundMaterialMatch) << "Found a color match - but the materials differ: " << gltfMaterial
											  << " versus " << mvPalette.material(foundColorMatch);
		}
	}
}

} // namespace voxelformat
