/**
 * @file
 */

#include "voxelformat/OBJFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "voxelformat/QBFormat.h"
#include "io/FileStream.h"

namespace voxelformat {

class OBJFormatTest : public AbstractVoxFormatTest {};

TEST_F(OBJFormatTest, testVoxelize) {
	OBJFormat f;
	const core::String filename = "cube.obj";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(OBJFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	{
		QBFormat sourceFormat;
		const core::String filename = "rgb.qb";
		const io::FilePtr &file = open(filename);
		io::FileStream stream(file);
		EXPECT_TRUE(sourceFormat.load(filename, stream, sceneGraph, testLoadCtx));
	}
	ASSERT_TRUE(sceneGraph.size() > 0);
	OBJFormat f;
	const core::String outFilename = "exportrgb.obj";
	const io::FilePtr &outFile = open(outFilename, io::FileMode::SysWrite);
	io::FileStream outStream(outFile);
	EXPECT_TRUE(f.saveGroups(sceneGraph, outFilename, outStream, testSaveCtx));
}

} // namespace voxel
