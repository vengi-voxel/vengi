/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../Util.h"
#include "../Parser.h"

class ShaderToolTest: public app::AbstractTest {
};

TEST_F(ShaderToolTest, testConvertName) {
	EXPECT_EQ("fooBar", util::convertName("foo_bar", false));
	EXPECT_EQ("FooBar", util::convertName("foo_bar", true));
}

TEST_F(ShaderToolTest, testParse) {
	ShaderStruct shaderStruct;
	core::String shaderFile = "testshader.glsl";
	const core::String buffer = _testApp->filesystem()->load(shaderFile);
	const bool vertex = true;
	ASSERT_TRUE(shadertool::parse("**unittest**", shaderStruct, shaderFile, buffer, vertex));
	ASSERT_EQ(shaderFile, shaderStruct.filename);
	ASSERT_EQ(2u, shaderStruct.uniforms.size());
	ASSERT_EQ(3u, shaderStruct.attributes.size());
	ASSERT_EQ(1u, shaderStruct.constants.size());
	ASSERT_EQ("FlagBloom", shaderStruct.constants.begin()->first);
	ASSERT_EQ("2u", shaderStruct.constants.begin()->value);
	ASSERT_EQ(2u, shaderStruct.layouts.size());
	ASSERT_EQ(1u, shaderStruct.uniformBlocks.size());
	ASSERT_EQ("u_materialblock", shaderStruct.uniformBlocks.begin()->value.name);
}
