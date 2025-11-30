/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

class QBCLFormatTest: public AbstractFormatTest {
};

TEST_F(QBCLFormatTest, testLoad) {
	testLoad("qubicle.qbcl", 30);
}

TEST_F(QBCLFormatTest, testSaveSmallVoxel) {
	QBCLFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbcl", &f, 0, 1, flags);
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
	testLoadScreenshot("chr_knight.qbcl", 100, 100, color::RGBA(147, 53, 53), 59, 1);
}

TEST_F(QBCLFormatTest, testLoadCrabby) {
	scenegraph::SceneGraph qbclsceneGraph;
	testLoad(qbclsceneGraph, "crabby.qbcl", 2);
	scenegraph::SceneGraph voxsceneGraph;
	testLoad(voxsceneGraph, "crabby.vox", 2);
	voxel::sceneGraphComparator(qbclsceneGraph, voxsceneGraph, voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

}
