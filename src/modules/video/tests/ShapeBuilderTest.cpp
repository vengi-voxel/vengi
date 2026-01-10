/**
 * @file
 */

#include "video/ShapeBuilder.h"
#include "app/tests/AbstractTest.h"

namespace video {

class ShapeBuilderTest : public app::AbstractTest {};

TEST_F(ShapeBuilderTest, testOBB) {
	ShapeBuilder shapeBuilder(100);
	math::OBBF obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
	shapeBuilder.obb(obb);
	EXPECT_EQ(8u, shapeBuilder.getVertices().size());
	EXPECT_EQ(24u, shapeBuilder.getIndices().size());
}

} // namespace video
