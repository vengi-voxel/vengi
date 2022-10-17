/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXLFormat.h"
#include <glm/gtx/transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

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
	for (int row = 0; row < glm::mat4::col_type::length(); ++row) {
		for (int col = 0; col < glm::mat4::length(); ++col) {
			EXPECT_FLOAT_EQ(m1[col][row], m2[col][row]) << "row " << row << ", col " << col << " differs: " << glm::to_string(m1) << glm::to_string(m2);
		}
	}
}

}
