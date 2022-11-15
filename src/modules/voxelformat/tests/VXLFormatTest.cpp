/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"
#include <glm/gtx/transform.hpp>

namespace voxelformat {

class VXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXLFormatTest, testLoad) {
	canLoad("cc.vxl");
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
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f);
}

TEST_F(VXLFormatTest, testSwitchYAndZ) {
	voxelformat::VXLFormat::VXLMatrix vxlMatrix;
	const glm::mat4 m1 = glm::translate(glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(10.0f, 20.0f, 30.0f));
	vxlMatrix.fromMat4(m1);
	const glm::mat4 m2 = vxlMatrix.toMat4();
	EXPECT_EQ(m1, m2) << vxlMatrix.matrix;
}

}
