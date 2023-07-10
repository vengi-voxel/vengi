/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
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
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(GLTFFormatTest, DISABLED_testVoxelizeLantern) {
	GLTFFormat f;
	const core::String filename = "glTF/lantern/Lantern.gltf";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

} // namespace voxelformat
