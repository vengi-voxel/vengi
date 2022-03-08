/**
 * @file
 */

#include "voxelformat/OBJFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "voxelformat/QBFormat.h"

namespace voxel {

class OBJFormatTest : public AbstractVoxFormatTest {};

TEST_F(OBJFormatTest, testVoxelize) {
	OBJFormat f;
	const core::String filename = "cube.obj";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(OBJFormatTest, testExportMesh) {
	SceneGraph sceneGraph;
	{
		QBFormat sourceFormat;
		const core::String filename = "rgb.qb";
		const io::FilePtr &file = open(filename);
		io::FileStream stream(file);
		EXPECT_TRUE(sourceFormat.loadGroups(filename, stream, sceneGraph));
	}
	ASSERT_TRUE(sceneGraph.size() > 0);
	OBJFormat f;
	const core::String outFilename = "exportrgb.obj";
	const io::FilePtr &outFile = open(outFilename, io::FileMode::SysWrite);
	io::FileStream outStream(outFile);
	EXPECT_TRUE(f.saveGroups(sceneGraph, outFilename, outStream));
}

} // namespace voxel
