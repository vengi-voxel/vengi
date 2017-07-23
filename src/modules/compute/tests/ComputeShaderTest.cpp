/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "TestsShaders.h"

namespace compute {

class ComputeShaderTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(compute::init());
	}

	void TearDown() override {
		compute::shutdown();
		Super::TearDown();
	}
};

TEST_F(ComputeShaderTest, testExecuteSimpleShader) {
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const char *foo = "1234";
	char foo2[4] = {};
	ASSERT_TRUE(shader.example(foo, strlen(foo), foo2, sizeof(foo2), strlen(foo), 1));
	ASSERT_EQ(std::string(foo), std::string(foo2, strlen(foo)));
}

TEST_F(ComputeShaderTest, testExecuteVectorAdd) {
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	std::vector<int> a(1000, 1);
	std::vector<int> b(1000, 2);
	std::vector<int> c(1000, 0);
	ASSERT_TRUE(shader.vector_add(&a.front(), &b.front(), &c.front(), a.size() / 2, 2));
	ASSERT_EQ(c[999], 3);
}

TEST_F(ComputeShaderTest, testExecuteVectorAddNormal) {
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	std::vector<int> a(1000, 1);
	std::vector<int> b(1000, 2);
	std::vector<int> c(1000, 0);
	for (int i = 0; i < 1000; ++i) {
		c[i] = a[i] + b[i];
	}
	ASSERT_EQ(c[999], 3);
}

}
