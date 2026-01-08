/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class VMaxFormatTest : public AbstractFormatTest {};

TEST_F(VMaxFormatTest, testLoad) {
	ASSERT_TRUE(io::isA("0voxel.vmax.zip", voxelformat::voxelLoad()));
}

TEST_F(VMaxFormatTest, DISABLED_testTransform) {
	// this is the same model as test-transform.vox but in vmax format
	scenegraph::SceneGraph sceneGraphVMAX;
	testTransform(sceneGraphVMAX, "test-transform.vmax.zip");

	scenegraph::SceneGraph sceneGraphVOX;
	testLoad(sceneGraphVOX, "test-transform.vox", 20);

	const voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	voxel::sceneGraphComparator(sceneGraphVMAX, sceneGraphVOX, flags);
}

TEST_F(VMaxFormatTest, testLoad0) {
	// Node 'snapshots' is empty - this scene doesn't contain anything
	testLoad("0voxel.vmax.zip", 0);
}

TEST_F(VMaxFormatTest, testLoad1) {
	testLoad("1voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad2) {
	testLoad("2voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad5) {
	testLoad("5voxel.vmax.zip");
}

TEST_F(VMaxFormatTest, testLoad5Screenshot) {
	color::RGBA color(251, 251, 251, 255);
	testLoadScreenshot("5voxel.vmax.zip", 1280, 1280, color, 1, 1);
}

} // namespace voxelformat
