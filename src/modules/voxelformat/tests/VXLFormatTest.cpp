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

TEST_F(VXLFormatTest, testSaveSmallVoxel) {
	VXLFormat f;
	testSaveLoadVoxel("cc-smallvolumesavetest.vxl", &f);
}

TEST_F(VXLFormatTest, testSwitchYAndZ) {
	class TestClass : public VXLFormat {
	public:
		glm::mat4 switchAxis (const glm::mat4 &in) {
			return convertToGLM(convertToWestwood(in));
		}
	} test;

	const glm::mat4 m1 = glm::translate(glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(10.0f, 20.0f, 30.0f));
	const glm::mat4 m2 = test.switchAxis(m1);
	EXPECT_EQ(m1, m2);
}

}
