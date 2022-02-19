/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/OBJFormat.h"

namespace voxel {

class OBJFormatTest: public AbstractVoxFormatTest {
};

TEST_F(OBJFormatTest, testVoxelize) {
	OBJFormat f;
	const core::String filename = "cube.obj";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph));
}

}
