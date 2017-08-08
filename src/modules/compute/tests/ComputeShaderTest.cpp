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
	ASSERT_TRUE(shader.example((const int8_t *)foo, strlen(foo), (int8_t *)foo2, sizeof(foo2), strlen(foo)));
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
	ASSERT_TRUE(shader.example2((const int8_t *)foo, strlen(foo), (int8_t *)foo2, sizeof(foo2), 42, strlen(foo)));
	ASSERT_EQ(std::string(foo), std::string(foo2, strlen(foo)));
}

TEST_F(ComputeShaderTest, testExecuteExampleBig) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	std::vector<int8_t> source(10000, 'a');
	std::vector<int8_t> target(10000, ' ');
	ASSERT_TRUE(shader.example(source.data(), source.size(), &target[0], target.size(), source.size()));
	ASSERT_EQ(source, target);
}

TEST_F(ComputeShaderTest, testExecuteExampleVectorAddFloat3NoPointer) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	float A[3] = {0.0f, 1.0f, 2.0f};
	float B[3] = {0.0f, 2.0f, 4.0f};
	float C[3] = {0.0f};
	ASSERT_TRUE(shader.exampleVectorAddFloat3NoPointer(A, B, C, 3));
	ASSERT_FLOAT_EQ(C[0], 0.0f);
	ASSERT_FLOAT_EQ(C[2], 6.0f);
}

#if 0
TEST_F(ComputeShaderTest, testExecuteExampleVectorAddFloat3) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	float A[2][3] = {{0.0f, 1.0f, 2.0f}, {0.0f, 1.0f, 2.0f}};
	float B[2][3] = {{0.0f, 2.0f, 4.0f}, {0.0f, 2.0f, 4.0f}};
	float C[2][3] = {{0.0f}, {0.0f}};
	ASSERT_TRUE(shader.exampleVectorAddFloat3(A, sizeof(A), B, sizeof(B), C, sizeof(C), 3, 1));
	ASSERT_FLOAT_EQ(C[0][0], 0.0f);
	ASSERT_FLOAT_EQ(C[2][1], 6.0f);
}
#endif

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
	ASSERT_TRUE(shader.exampleVectorAddInt(&a.front(), core::vectorSize(a), &b.front(), core::vectorSize(b), &c.front(), core::vectorSize(c), size));
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
