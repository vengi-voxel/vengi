/**
 * @file
 */

#include "video/ShapeBuilder.h"
#include "app/tests/AbstractTest.h"
#include "math/Mat3f.h"
#include "math/Vec3f.h"

namespace video {

class ShapeBuilderTest : public app::AbstractTest {};

TEST_F(ShapeBuilderTest, testOBB) {
	ShapeBuilder shapeBuilder(100);
	math::OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
	shapeBuilder.obb(obb);
	EXPECT_EQ(8u, shapeBuilder.getVertices().size());
	EXPECT_EQ(24u, shapeBuilder.getIndices().size());
}

TEST_F(ShapeBuilderTest, testClearResetsPrimitiveForBones) {
	ShapeBuilder shapeBuilder(256);
	shapeBuilder.obb(math::OBBF(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f)));
	EXPECT_EQ(Primitive::Lines, shapeBuilder.primitive());
	shapeBuilder.clear();
	EXPECT_EQ(Primitive::Triangles, shapeBuilder.primitive());
	shapeBuilder.bone(math::Vec3f(0.0f), math::Vec3f(0.0f, 0.0f, 8.0f));
	EXPECT_EQ(Primitive::Triangles, shapeBuilder.primitive());
	EXPECT_GT(shapeBuilder.getIndices().size(), 0u);
	EXPECT_EQ(0u, shapeBuilder.getIndices().size() % 3u);
}

} // namespace video
