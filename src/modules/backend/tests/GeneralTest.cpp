#include "TestShared.h"

class GeneralTest: public TestSuite {
};

TEST_F(GeneralTest, testMathParseVec3) {
	glm::vec3 v1;
	EXPECT_TRUE(backend::parse("0:0:0", v1));
	EXPECT_FLOAT_EQ(0.0f, v1.x);
	EXPECT_FLOAT_EQ(0.0f, v1.y);
	EXPECT_FLOAT_EQ(0.0f, v1.z);

	glm::vec3 v2;
	EXPECT_TRUE(backend::parse("100:0:0", v2));
	EXPECT_FLOAT_EQ(100.0f, v2.x);
	EXPECT_FLOAT_EQ(0.0f, v2.y);
	EXPECT_FLOAT_EQ(0.0f, v2.z);

	glm::vec3 v3;
	EXPECT_TRUE(backend::parse("100:-100:42", v3));
	EXPECT_FLOAT_EQ(100.0f, v3.x);
	EXPECT_FLOAT_EQ(-100.0f, v3.y);
	EXPECT_FLOAT_EQ(42.0f, v3.z);
}
