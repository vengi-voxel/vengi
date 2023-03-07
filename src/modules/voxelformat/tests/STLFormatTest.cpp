/**
 * @file
 */

#include "voxelformat/STLFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace voxelformat {

class STLFormatTest : public AbstractVoxFormatTest {};

TEST_F(STLFormatTest, testVoxelizeAscii) {
	STLFormat f;
	const core::String filename = "ascii.stl";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(STLFormatTest, testVoxelizeCube) {
	STLFormat f;
	const core::String filename = "cube.stl";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

} // namespace voxel
