/**
 * @file
 */

#include "TestHelper.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/CoordinateSystemUtil.h"

namespace scenegraph {

class CoordinateSystemTest : public app::AbstractTest {
protected:
	void testConvert(CoordinateSystem from, CoordinateSystem to) {
		glm::mat4 identity(1.0f);
		const glm::mat4 &toMatrix = convertCoordinateSystem(from, to, identity);
		const glm::mat4 &fromMatrix = convertCoordinateSystem(to, from, toMatrix);
		EXPECT_EQ(identity, fromMatrix);
	}
};

TEST_F(CoordinateSystemTest, testVXL) {
	testConvert(CoordinateSystem::VXL, CoordinateSystem::Vengi);
}

TEST_F(CoordinateSystemTest, testMagicavoxel) {
	testConvert(CoordinateSystem::MagicaVoxel, CoordinateSystem::Vengi);
}

} // namespace scenegraph
