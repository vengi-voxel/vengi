/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "TestsShaders.h"

namespace compute {

class ComputeShaderTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	bool _supported = false;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = compute::init();
	}

	void TearDown() override {
		compute::shutdown();
		Super::TearDown();
	}
};

TEST_F(ComputeShaderTest, testExecuteExample) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const char *foo = "1234";
	char foo2[4] = {};
	ASSERT_TRUE(shader.example(foo, strlen(foo), foo2, sizeof(foo2), strlen(foo), 1));
	ASSERT_EQ(std::string(foo), std::string(foo2, strlen(foo)));
}

TEST_F(ComputeShaderTest, testExecuteExample2) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const char *foo = "1234";
	char foo2[4] = {};
	ASSERT_TRUE(shader.example2(foo, strlen(foo), foo2, sizeof(foo2), 42, strlen(foo), 1));
	ASSERT_EQ(std::string(foo), std::string(foo2, strlen(foo)));
}

TEST_F(ComputeShaderTest, testExecuteExampleBig) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	std::vector<char> source(10000, 'a');
	std::vector<char> target(10000, ' ');
	ASSERT_TRUE(shader.example(source.data(), source.size(), &target[0], target.size(), source.size(), 1));
	ASSERT_EQ(source, target);
}

// just for comparing runtimes
TEST_F(ComputeShaderTest, testExecuteExampleBigNonOpenCL) {
	std::vector<char> source(10000, 'a');
	std::vector<char> target(10000, ' ');
	for (int i = 0; i < 10000; ++i) {
		source[i] = target[i];
	}
	ASSERT_EQ(source, target);
}

TEST_F(ComputeShaderTest, testExecuteVectorAdd) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	constexpr int size = 1000;
	ASSERT_GT(size, 2);
	constexpr int initA = 1;
	constexpr int initB = 2;
	std::vector<int> a(size, initA);
	std::vector<int> b(size, initB);
	std::vector<int> c(size, 0);
	ASSERT_TRUE(shader.vector_add(&a.front(), core::vectorSize(a), &b.front(), core::vectorSize(b), &c.front(), core::vectorSize(c), size, 1));
	for (int i = 0; i < size; ++i) {
		SCOPED_TRACE(core::string::format("index: %i", i));
		ASSERT_EQ(c[i], initA + initB);
	}
}

// just for comparing runtimes
TEST_F(ComputeShaderTest, testExecuteVectorAddNonOpenCL) {
	constexpr int size = 1000;
	ASSERT_GT(size, 2);
	constexpr int initA = 1;
	constexpr int initB = 2;
	std::vector<int> a(size, initA);
	std::vector<int> b(size, initB);
	std::vector<int> c(size, 0);
	for (int i = 0; i < size; ++i) {
		c[i] = a[i] + b[i];
	}
	for (int i = 0; i < size; ++i) {
		SCOPED_TRACE(core::string::format("index: %i", i));
		ASSERT_EQ(c[i], initA + initB);
	}
}

}
