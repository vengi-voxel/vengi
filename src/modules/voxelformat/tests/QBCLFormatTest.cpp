/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/QBCLFormat.h"
#include "scenegraph/SceneGraphNode.h"
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

TEST_F(QBCLFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.qbcl", 100, 100, core::RGBA(147, 53, 53), 59, 1);
}

TEST_F(QBCLFormatTest, testLoadCrabby) {
	voxelformat::SceneGraph qbclsceneGraph;
	{
		const core::String filename = "crabby.qbcl";
		const io::FilePtr& file = open(filename);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		ASSERT_TRUE(voxelformat::loadFormat(filename, stream, qbclsceneGraph, testLoadCtx));
	}
	voxelformat::SceneGraph voxsceneGraph;
	{
		const core::String filename = "crabby.vox";
		const io::FilePtr& file = open(filename);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		ASSERT_TRUE(voxelformat::loadFormat(filename, stream, voxsceneGraph, testLoadCtx));
	}
	voxel::sceneGraphComparator(qbclsceneGraph, voxsceneGraph, voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

}
