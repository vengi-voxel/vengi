/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "io/FileStream.h"

namespace voxelformat {

class QBCLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBCLFormatTest, testLoad) {
	canLoad("qubicle.qbcl", 30);
}

TEST_F(QBCLFormatTest, testSaveSmallVoxel) {
	QBCLFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbcl", &f);
}

TEST_F(QBCLFormatTest, testLoadRGB) {
	testRGB("rgb.qbcl");
}

TEST_F(QBCLFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qbcl");
}

TEST_F(QBCLFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qbcl");
}

// TODO: still fails - most likely related to the mv palette index 0
TEST_F(QBCLFormatTest, DISABLED_testLoadCrabby) {
	voxelformat::SceneGraph qbclsceneGraph;
	{
		const core::String filename = "crabby.qbcl";
		const io::FilePtr& file = open(filename);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		ASSERT_TRUE(voxelformat::loadFormat(filename, stream, qbclsceneGraph));
		const SceneGraphNode* node = qbclsceneGraph.findNodeByName("Matrix");
		ASSERT_NE(nullptr, node);
		qbclsceneGraph.removeNode(node->id(), false);
	}
	voxelformat::SceneGraph voxsceneGraph;
	{
		const core::String filename = "crabby.vox";
		const io::FilePtr& file = open(filename);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		ASSERT_TRUE(voxelformat::loadFormat(filename, stream, voxsceneGraph));
	}
	voxel::sceneGraphComparator(qbclsceneGraph, voxsceneGraph, voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

}
