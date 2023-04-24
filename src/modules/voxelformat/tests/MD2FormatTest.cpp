/**
 * @file
 */

#include "voxelformat/MD2Format.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace voxelformat {

class MD2FormatTest : public AbstractVoxFormatTest {};

TEST_F(MD2FormatTest, DISABLED_testVoxelize) {
	MD2Format f;
	const core::String filename = "cube.md2";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

} // namespace voxelformat
