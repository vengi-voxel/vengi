/**
 * @file
 */

#include "math/Bezier.h"
#include "app/tests/AbstractTest.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace math {

class BezierTest : public app::AbstractTest {};

TEST_F(BezierTest, testGetPoint) {
	Bezier<float> b(math::Vec3f(0.0f), math::Vec3f(10.0f, 0.0f, 0.0f), math::Vec3f(5.0f, 5.0f, 0.0f));
	math::Vec3f p = b.getPoint(0.0f);
	EXPECT_FLOAT_EQ(0.5f, p.x);
	EXPECT_FLOAT_EQ(0.5f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);

	p = b.getPoint(0.5f);
	EXPECT_FLOAT_EQ(5.5f, p.x);
	EXPECT_FLOAT_EQ(3.0f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);

	p = b.getPoint(1.0f);
	EXPECT_FLOAT_EQ(10.5f, p.x);
	EXPECT_FLOAT_EQ(0.5f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);
}

TEST_F(BezierTest, testEvaluate) {
	Bezier<float> b(math::Vec3f(0.0f), math::Vec3f(10.0f, 0.0f, 0.0f), math::Vec3f(5.0f, 5.0f, 0.0f));
	const math::Vec3f p = b.evaluate(0.5f);
	EXPECT_FLOAT_EQ(5.0f, p.x);
	EXPECT_FLOAT_EQ(2.5f, p.y);
	EXPECT_FLOAT_EQ(0.0f, p.z);
}

TEST_F(BezierTest, testVisitSegments) {
	Bezier<int> b(math::Vec3i(0), math::Vec3i(10, 0, 0), math::Vec3i(5, 5, 0));
	math::Vec3i firstFrom(-1);
	math::Vec3i lastTo(-1);
	int segments = 0;
	b.visitSegments(4, [&](const math::Vec3i &from, const math::Vec3i &to) {
		if (segments == 0) {
			firstFrom = from;
		}
		lastTo = to;
		++segments;
	});
	EXPECT_EQ(4, segments);
	EXPECT_EQ(math::Vec3i(0), firstFrom);
	EXPECT_EQ(math::Vec3i(10, 0, 0), lastTo);
	EXPECT_EQ(16, b.estimateSteps());
}

} // namespace math
