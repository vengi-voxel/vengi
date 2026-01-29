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

TEST_F(ShaderToolTest, testParseUniformBlock) {
	ShaderStruct shaderStruct;
	core::String shaderFile = "testshader.glsl";
	const core::String buffer = _testApp->filesystem()->load(shaderFile);
	const bool vertex = true;
	ASSERT_TRUE(shadertool::parse("**unittest**", shaderStruct, shaderFile, buffer, vertex));

	// Verify uniform block parsing
	ASSERT_EQ(1u, shaderStruct.uniformBlocks.size());
	const BufferBlock& ubo = shaderStruct.uniformBlocks.begin()->value;
	EXPECT_EQ("u_materialblock", ubo.name);
	EXPECT_EQ(BlockLayout::std140, ubo.layout.blockLayout);
	EXPECT_EQ(8u, ubo.members.size());

	// Check individual members
	auto it = ubo.members.begin();
	EXPECT_EQ("u_materialcolor", it->value.name);
	EXPECT_EQ(Variable::VEC4, it->value.type);
	EXPECT_EQ(256, it->value.arraySize);
	++it;
	EXPECT_EQ("u_glowcolor", it->value.name);
	EXPECT_EQ(Variable::VEC4, it->value.type);
	EXPECT_EQ(256, it->value.arraySize);
}

TEST_F(ShaderToolTest, testParseSSBO) {
	ShaderStruct shaderStruct;
	core::String shaderFile = "testshader.glsl";
	const core::String buffer = _testApp->filesystem()->load(shaderFile);
	const bool vertex = true;
	ASSERT_TRUE(shadertool::parse("**unittest**", shaderStruct, shaderFile, buffer, vertex));

	// Verify SSBO parsing
	ASSERT_EQ(2u, shaderStruct.bufferBlocks.size());

	// Find ParticleBuffer
	bool foundParticleBuffer = false;
	bool foundTransformBuffer = false;
	for (const auto& buf : shaderStruct.bufferBlocks) {
		if (buf.name == "ParticleBuffer") {
			foundParticleBuffer = true;
			EXPECT_EQ(BlockLayout::std430, buf.layout.blockLayout);
			EXPECT_EQ(0, buf.layout.binding);
			EXPECT_EQ(3u, buf.members.size());

			auto it = buf.members.begin();
			EXPECT_EQ("positions", it->value.name);
			EXPECT_EQ(Variable::VEC4, it->value.type);
			EXPECT_EQ(64, it->value.arraySize);
			++it;
			EXPECT_EQ("velocities", it->value.name);
			EXPECT_EQ(Variable::VEC4, it->value.type);
			EXPECT_EQ(64, it->value.arraySize);
			++it;
			EXPECT_EQ("masses", it->value.name);
			EXPECT_EQ(Variable::FLOAT, it->value.type);
			EXPECT_EQ(-1, it->value.arraySize); // Dynamic array
		} else if (buf.name == "TransformBuffer") {
			foundTransformBuffer = true;
			EXPECT_EQ(BlockLayout::std430, buf.layout.blockLayout);
			EXPECT_EQ(1, buf.layout.binding);
			EXPECT_EQ(3u, buf.members.size());

			auto it = buf.members.begin();
			EXPECT_EQ("transforms", it->value.name);
			EXPECT_EQ(Variable::MAT4, it->value.type);
			EXPECT_EQ(16, it->value.arraySize);
			++it;
			EXPECT_EQ("count", it->value.name);
			EXPECT_EQ(Variable::INT, it->value.type);
			EXPECT_EQ(0, it->value.arraySize);
			++it;
			EXPECT_EQ("flags", it->value.name);
			EXPECT_EQ(Variable::UNSIGNED_INT, it->value.type);
			EXPECT_EQ(0, it->value.arraySize);
		}
	}
	EXPECT_TRUE(foundParticleBuffer) << "ParticleBuffer SSBO not found";
	EXPECT_TRUE(foundTransformBuffer) << "TransformBuffer SSBO not found";
}

TEST_F(ShaderToolTest, testStd430Alignment) {
	// Test std430 alignment calculations
	Variable floatVar;
	floatVar.type = Variable::FLOAT;
	floatVar.arraySize = 0;
	EXPECT_EQ(1, util::std430Align(floatVar));
	EXPECT_EQ(1u, util::std430Size(floatVar));

	Variable vec2Var;
	vec2Var.type = Variable::VEC2;
	vec2Var.arraySize = 0;
	EXPECT_EQ(2, util::std430Align(vec2Var));
	EXPECT_EQ(2u, util::std430Size(vec2Var));

	Variable vec3Var;
	vec3Var.type = Variable::VEC3;
	vec3Var.arraySize = 0;
	EXPECT_EQ(4, util::std430Align(vec3Var));
	EXPECT_EQ(3u, util::std430Size(vec3Var));

	Variable vec4Var;
	vec4Var.type = Variable::VEC4;
	vec4Var.arraySize = 0;
	EXPECT_EQ(4, util::std430Align(vec4Var));
	EXPECT_EQ(4u, util::std430Size(vec4Var));

	Variable mat4Var;
	mat4Var.type = Variable::MAT4;
	mat4Var.arraySize = 0;
	EXPECT_EQ(4, util::std430Align(mat4Var));
	EXPECT_EQ(16u, util::std430Size(mat4Var));

	// Test arrays
	Variable vec4ArrayVar;
	vec4ArrayVar.type = Variable::VEC4;
	vec4ArrayVar.arraySize = 4;
	EXPECT_EQ(16u, util::std430Size(vec4ArrayVar)); // 4 * 4 = 16
}
