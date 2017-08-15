/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "compute/Types.h"
#include "../Util.h"

class ComputeShaderToolTest: public core::AbstractTest {
};

TEST_F(ComputeShaderToolTest, testConvertType) {
	std::string arrayDefinition;
	int arraySize = 0;

	EXPECT_EQ("float", util::convertType("float", arrayDefinition, &arraySize));
	EXPECT_EQ(0, arraySize);
	EXPECT_EQ("", arrayDefinition);

	EXPECT_EQ("float", util::convertType("float2", arrayDefinition, &arraySize));
	EXPECT_EQ(2, arraySize);
	EXPECT_EQ("[2]", arrayDefinition);

	EXPECT_EQ("float", util::convertType("float3", arrayDefinition, &arraySize));
	EXPECT_EQ(3, arraySize);
	EXPECT_EQ("[3]", arrayDefinition);

	EXPECT_EQ("float", util::convertType("float4", arrayDefinition, &arraySize));
	EXPECT_EQ(4, arraySize);
	EXPECT_EQ("[4]", arrayDefinition);

	EXPECT_EQ("float *", util::convertType("float2*", arrayDefinition, &arraySize));
	EXPECT_EQ(2, arraySize);
	EXPECT_EQ("[2]", arrayDefinition);

	EXPECT_EQ("float *", util::convertType("float3*", arrayDefinition, &arraySize));
	EXPECT_EQ(3, arraySize);
	EXPECT_EQ("[3]", arrayDefinition);

	EXPECT_EQ("float *", util::convertType("float4*", arrayDefinition, &arraySize));
	EXPECT_EQ(4, arraySize);
	EXPECT_EQ("[4]", arrayDefinition);
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
