/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "compute/Types.h"
#include "../Util.h"

namespace computeshadertool {

class ComputeShaderToolTest: public core::AbstractTest {
};

TEST_F(ComputeShaderToolTest, testConvertVectorType) {
	EXPECT_EQ("uint8_t", util::vectorType("uchar").type);
	EXPECT_EQ("uint8_t", util::vectorType("uchar4").type);
	EXPECT_EQ(4, util::vectorType("uchar4").arraySize);
	EXPECT_EQ("int8_t", util::vectorType("char").type);
	EXPECT_EQ("float", util::vectorType("float").type);
	EXPECT_EQ("glm::vec2", util::vectorType("float2").type);
	EXPECT_EQ("glm::vec3", util::vectorType("float3").type);
	EXPECT_EQ("glm::vec4", util::vectorType("float4").type);
	EXPECT_EQ("glm::vec2", util::vectorType("float2*").type);
	EXPECT_EQ("glm::vec3", util::vectorType("float3*").type);
	EXPECT_EQ("glm::vec4", util::vectorType("float4*").type);
	EXPECT_EQ("glm::vec2", util::vectorType("float2 *").type);
	EXPECT_EQ("glm::vec3", util::vectorType("float3 *").type);
	EXPECT_EQ("glm::vec4", util::vectorType("float4 *").type);
}

TEST_F(ComputeShaderToolTest, testIsQualifier) {
	EXPECT_TRUE(util::isQualifier("const"));
	EXPECT_TRUE(util::isQualifier("__constant"));
}

TEST_F(ComputeShaderToolTest, testToString) {
	EXPECT_EQ("compute::BufferFlag::ReadWrite", util::toString(compute::BufferFlag::ReadWrite));
	EXPECT_EQ("compute::BufferFlag::ReadWrite | compute::BufferFlag::ReadOnly",
			util::toString(compute::BufferFlag::ReadWrite | compute::BufferFlag::ReadOnly));
}

}
