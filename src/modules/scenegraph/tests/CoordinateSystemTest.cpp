/**
 * @file
 */

#include "TestHelper.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/CoordinateSystemUtil.h"

namespace scenegraph {

class CoordinateSystemTest : public app::AbstractTest {
protected:
	void testConvertIdentity(CoordinateSystem from, CoordinateSystem to) {
		glm::mat4 identity(1.0f);
		const glm::mat4 &toMatrix = convertCoordinateSystem(from, to, identity);
		const glm::mat4 &fromMatrix = convertCoordinateSystem(to, from, toMatrix);
		EXPECT_EQ(identity, fromMatrix);
	}
	void testConvert(CoordinateSystem from, CoordinateSystem to) {
		glm::mat4 src = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)),
											   glm::radians(23.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
								   glm::vec3(2.0f, 3.0f, 4.0f));
		const glm::mat4 &toMatrix = convertCoordinateSystem(from, to, src);
		const glm::mat4 &fromMatrix = convertCoordinateSystem(to, from, toMatrix);
		EXPECT_EQ(src, fromMatrix);
	}
};

TEST_F(CoordinateSystemTest, testVXL) {
	testConvertIdentity(CoordinateSystem::VXL, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::VXL, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testMagicavoxel) {
	testConvertIdentity(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi);
	testConvert(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi);
}

} // namespace scenegraph
