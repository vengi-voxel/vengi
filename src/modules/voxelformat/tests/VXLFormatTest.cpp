/**
 * @file
 */

#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "AbstractFormatTest.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxelformat {

class VXLFormatTest : public AbstractFormatTest {};

TEST_F(VXLFormatTest, testLoad) {
	testLoad("cc.vxl");
}

TEST_F(VXLFormatTest, testLoadRGB) {
	testRGB("rgb.vxl");
}

TEST_F(VXLFormatTest, testSaveAndLoadSceneGraph) {
	VXLFormat f;
	testLoadSaveAndLoadSceneGraph("cc.vxl", f, "cc-save.vxl", f);
}

TEST_F(VXLFormatTest, DISABLED_testSaveAndLoadSceneGraphWithAnimations) {
	VXLFormat f;
	testLoadSaveAndLoadSceneGraph("hmec.vxl", f, "hmec-save.vxl", f);
}

TEST_F(VXLFormatTest, testSaveSmallVoxel) {
	VXLFormat f;
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f, 0, 1,
					  voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette);
}

} // namespace voxelformat
