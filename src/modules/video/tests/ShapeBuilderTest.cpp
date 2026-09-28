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

TEST_F(ShapeBuilderTest, testClearResetsPrimitiveForBones) {
	ShapeBuilder shapeBuilder(256);
	shapeBuilder.obb(math::OBBF(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f)));
	EXPECT_EQ(Primitive::Lines, shapeBuilder.primitive());
	shapeBuilder.clear();
	EXPECT_EQ(Primitive::Triangles, shapeBuilder.primitive());
	shapeBuilder.bone(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 8.0f));
	EXPECT_EQ(Primitive::Triangles, shapeBuilder.primitive());
	EXPECT_GT(shapeBuilder.getIndices().size(), 0u);
	EXPECT_EQ(0u, shapeBuilder.getIndices().size() % 3u);
}

} // namespace video
