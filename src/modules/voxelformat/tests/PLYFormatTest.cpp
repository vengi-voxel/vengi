/**
 * @file
 */

#include "voxelformat/private/mesh/PLYFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace voxelformat {

class PLYFormatTest : public AbstractVoxFormatTest {};

TEST_F(PLYFormatTest, testVoxelizeAscii) {
	PLYFormat f;
	const core::String filename = "ascii.ply";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

TEST_F(PLYFormatTest, testVoxelizeCube) {
	PLYFormat f;
	const core::String filename = "cube.ply";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

} // namespace voxel
