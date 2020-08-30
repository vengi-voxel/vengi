/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "TestsComputeShaders.h"

namespace compute {

class ComputeShaderTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
protected:
	bool _supported = false;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = compute::init();
		if (!_supported) {
			Log::warn("ComputeShaderTest is skipped");
		}
	}

	void TearDown() override {
		compute::shutdown();
		Super::TearDown();
	}
};

TEST_F(ComputeShaderTest, testCompileStruct) {
	compute::TestShader::Data data;
	EXPECT_EQ(sizeof(int32_t), sizeof(data.foo_int32_t));
	EXPECT_EQ(sizeof(int8_t), sizeof(data.foo2_int8_t));
	EXPECT_EQ(sizeof(int8_t) * 4, sizeof(data.foo2_char4));
	EXPECT_EQ(sizeof(float), sizeof(data.foo3_float));
	EXPECT_EQ(sizeof(glm::vec2), sizeof(data.foo3_vec2));
	EXPECT_EQ(sizeof(glm::vec3), sizeof(data.foo3_vec4));
	EXPECT_EQ(sizeof(glm::vec4), sizeof(data.foo4_vec4));
	EXPECT_EQ(sizeof(glm::vec4) * 2, sizeof(data.foo4_vec4_2));
	EXPECT_EQ(96u, sizeof(data));
}

TEST_F(ComputeShaderTest, testExecuteExample) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const std::vector<int8_t> foo { '1', '2', '3', '4', '5', '6' };
	std::vector<int8_t> foo2(foo.size(), '0');
	ASSERT_TRUE(shader.example(foo, foo2, glm::ivec1(foo.size())));
	ASSERT_EQ(foo, foo2);
}

TEST_F(ComputeShaderTest, testExecuteExample2) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const std::vector<int8_t> foo { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };
	std::vector<int8_t> foo2(foo.size(), '0');
	ASSERT_TRUE(shader.example2(foo, foo2, 42, glm::ivec1(foo.size())));
	EXPECT_EQ(foo, foo2);
}

TEST_F(ComputeShaderTest, testExecuteExampleBig) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	constexpr int size = 10000;
	std::vector<int8_t> source(size, 'a');
	std::vector<int8_t> target(size, ' ');
	ASSERT_TRUE(shader.example(source, target, glm::ivec1(source.size())));
	EXPECT_EQ(source, target);
}

TEST_F(ComputeShaderTest, testExecuteExampleVectorAddFloat3NoPointer) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const glm::vec3 A(0.0f, 1.0f, 2.0f);
	const glm::vec3 B(0.0f, 2.0f, 4.0f);
	glm::vec3 C(0.0f);
	ASSERT_TRUE(shader.exampleVectorAddFloat3NoPointer(A, B, C, glm::ivec1(3)));
#if 0
	// TODO: this kernel is a nop
	EXPECT_FLOAT_EQ(C.x, 0.0f);
	EXPECT_FLOAT_EQ(C.y, 3.0f);
	EXPECT_FLOAT_EQ(C.z, 6.0f);
#endif
}

TEST_F(ComputeShaderTest, testExecuteExampleVectorAddFloat3) {
	if (!_supported) {
		return;
	}
	compute::TestShader shader;
	ASSERT_TRUE(shader.setup());
	const std::vector<glm::vec3> A {glm::vec3{0.0f, 1.0f, 2.0f}, glm::vec3{0.0f, 1.0f, 2.0f}};
	const std::vector<glm::vec3> B {glm::vec3{0.0f, 2.0f, 4.0f}, glm::vec3{0.0f, 2.0f, 4.0f}};
	std::vector<glm::vec3> C(2);
	ASSERT_TRUE(shader.exampleVectorAddFloat3(A, B, C, glm::ivec1(3)));
	ASSERT_FLOAT_EQ(C[0][0], 0.0f);
	ASSERT_FLOAT_EQ(C[2][1], 6.0f);
}

// just for comparing runtimes
TEST_F(ComputeShaderTest, testExecuteExampleBigNonOpenCL) {
	std::vector<char> source(10000, 'a');
	std::vector<char> target(10000, ' ');
	for (int i = 0; i < 10000; ++i) {
		source[i] = target[i];
	}
	EXPECT_EQ(source, target);
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
	ASSERT_TRUE(shader.exampleVectorAddInt(a, b, c, glm::ivec1(size)));
	for (int i = 0; i < size; ++i) {
		SCOPED_TRACE(core::string::format("index: %i", i));
		EXPECT_EQ(c[i], initA + initB);
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
		EXPECT_EQ(c[i], initA + initB);
	}
}

}
