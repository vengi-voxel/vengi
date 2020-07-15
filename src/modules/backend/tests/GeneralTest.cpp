#include "TestShared.h"

class GeneralTest: public TestSuite {
};

TEST_F(GeneralTest, testMathParseVec3) {
	const glm::vec3 v1 = backend::parse("0:0:0");
	EXPECT_FLOAT_EQ(0.0f, v1.x);
	EXPECT_FLOAT_EQ(0.0f, v1.y);
	EXPECT_FLOAT_EQ(0.0f, v1.z);

	const glm::vec3 v2 = backend::parse("100:0:0");
	EXPECT_FLOAT_EQ(100.0f, v2.x);
	EXPECT_FLOAT_EQ(0.0f, v2.y);
	EXPECT_FLOAT_EQ(0.0f, v2.z);

	const glm::vec3 v3 = backend::parse("100:-100:42");
	EXPECT_FLOAT_EQ(100.0f, v3.x);
	EXPECT_FLOAT_EQ(-100.0f, v3.y);
	EXPECT_FLOAT_EQ(42.0f, v3.z);
}

TEST_F(GeneralTest, testMath) {
	const glm::vec3& angle = backend::fromRadians(glm::pi<float>());
	ASSERT_FLOAT_EQ(-1.0f, angle.x);
	ASSERT_FLOAT_EQ(0.0f, angle.y);
	ASSERT_FLOAT_EQ(glm::pi<float>(), glm::abs(backend::angle(angle)));
}
