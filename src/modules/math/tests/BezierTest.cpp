/**
 * @file
 */

#include "math/Bezier.h"
#include "app/tests/AbstractTest.h"

namespace math {

class BezierTest : public app::AbstractTest {};

TEST_F(BezierTest, testGetPoint) {
	Bezier<float> b(glm::vec3(0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(5.0f, 5.0f, 0.0f));
	glm::vec3 p = b.getPoint(0.0f);
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
	Bezier<float> b(glm::vec3(0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(5.0f, 5.0f, 0.0f));
	const glm::vec3 p = b.evaluate(0.5f);
	EXPECT_FLOAT_EQ(5.0f, p.x);
	EXPECT_FLOAT_EQ(2.5f, p.y);
	EXPECT_FLOAT_EQ(0.0f, p.z);
}

TEST_F(BezierTest, testVisitSegments) {
	Bezier<int> b(glm::ivec3(0), glm::ivec3(10, 0, 0), glm::ivec3(5, 5, 0));
	glm::ivec3 firstFrom(-1);
	glm::ivec3 lastTo(-1);
	int segments = 0;
	b.visitSegments(4, [&](const glm::ivec3 &from, const glm::ivec3 &to) {
		if (segments == 0) {
			firstFrom = from;
		}
		lastTo = to;
		++segments;
	});
	EXPECT_EQ(4, segments);
	EXPECT_EQ(glm::ivec3(0), firstFrom);
	EXPECT_EQ(glm::ivec3(10, 0, 0), lastTo);
	EXPECT_EQ(16, b.estimateSteps());
}

} // namespace math
