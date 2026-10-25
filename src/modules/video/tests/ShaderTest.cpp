/**
 * @file
 */

#include "video/Shader.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "video/ShaderStorageBuffer.h"
#include "video/tests/AbstractGLTest.h"

namespace video {

class ShaderTest : public app::AbstractTest {};

class IndirectComputeTest : public AbstractGLTest {};

TEST_F(IndirectComputeTest, testDispatchOffsetAndEmptyQueue) {
	if (!hasFeature(Feature::ComputeShaders)) {
		GTEST_SKIP() << "Compute shaders required";
	}
	Shader shader;
	ASSERT_TRUE(shader.load("indirect-dispatch-test", R"(
layout(local_size_x = 8) in;
layout(std430, binding = 0) buffer Results { uint values[]; };
void main() { values[gl_GlobalInvocationID.x] = gl_GlobalInvocationID.x + 1u; }
)",
							ShaderType::Compute));
	ASSERT_TRUE(shader.init());
	ShaderStorageBuffer arguments;
	const uint32_t counts[] = {0u, 1u, 1u, 0u, 3u, 1u, 1u};
	ASSERT_TRUE(arguments.create(counts, sizeof(counts)));
	ShaderStorageBuffer output;
	const uint32_t zeros[24] = {};
	ASSERT_TRUE(output.create(zeros, sizeof(zeros)));
	ASSERT_TRUE(output.bind(0));
	EXPECT_FALSE(runShaderIndirect(shader.handle(), InvalidId, 0u));
	EXPECT_FALSE(runShaderIndirect(shader.handle(), arguments.handle(), 1u));
	ASSERT_TRUE(runShaderIndirect(shader.handle(), arguments.handle(), 0u, MemoryBarrierType::All));
	const uint32_t *empty = (const uint32_t *)output.map(AccessMode::Read);
	ASSERT_NE(nullptr, empty);
	for (uint32_t i = 0; i < 24u; ++i) {
		EXPECT_EQ(0u, empty[i]);
	}
	output.unmap();
	ASSERT_TRUE(runShaderIndirect(shader.handle(), arguments.handle(), 4u * sizeof(uint32_t), MemoryBarrierType::All));
	const uint32_t *values = (const uint32_t *)output.map(AccessMode::Read);
	ASSERT_NE(nullptr, values);
	for (uint32_t i = 0; i < 24u; ++i) {
		EXPECT_EQ(i + 1u, values[i]);
	}
	output.unmap();
	output.shutdown();
	arguments.shutdown();
	shader.shutdown();
}

TEST_F(ShaderTest, testInclude) {
	const io::FilesystemPtr &filesystem = _testApp->filesystem();

	filesystem->homeWrite("foobar.vert", "#define SUCCESS");
	filesystem->homeWrite("foobar.frag", "#define SUCCESS");

	Shader s;
	const core::String &vert = s.getSource(ShaderType::Vertex, "#include \"foobar.vert\"");
	const core::String &frag = s.getSource(ShaderType::Fragment, "#include \"foobar.frag\"");
	ASSERT_TRUE(core::string::contains(vert, "SUCCESS")) << "vertex shader: " << vert;
	ASSERT_TRUE(core::string::contains(frag, "SUCCESS")) << "fragment shader: " << frag;
}

TEST_F(ShaderTest, testStripUnsupportedEsslRemovesNoperspective) {
	const core::String src = "noperspective in float v_edge;\nflat in float v_keep;\n";
	const core::String out = Shader::stripUnsupportedEssl(src);
	EXPECT_FALSE(core::string::contains(out, "noperspective")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "in float v_edge")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "flat in float v_keep")) << out.c_str();
}

TEST_F(ShaderTest, testGetSourceEsslStripsNoperspective) {
	const bool savedEssl = Shader::essl;
	const int savedVersion = Shader::glslVersion;
	Shader::essl = true;
	Shader::glslVersion = 300;
	Shader s;
	const core::String out = s.getSource(ShaderType::Vertex, "noperspective out float v_edge;\nflat out float v_keep;\n", true);
	Shader::essl = savedEssl;
	Shader::glslVersion = savedVersion;
	EXPECT_TRUE(core::string::contains(out, "#version 300 es")) << out.c_str();
	EXPECT_FALSE(core::string::contains(out, "noperspective")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "out float v_edge")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "flat out float v_keep")) << out.c_str();
}

TEST_F(ShaderTest, testGetSourceDesktopKeepsNoperspective) {
	const bool savedEssl = Shader::essl;
	const int savedVersion = Shader::glslVersion;
	Shader::essl = false;
	Shader::glslVersion = 430;
	Shader s;
	const core::String out = s.getSource(ShaderType::Vertex, "noperspective out float v_edge;\n", true);
	Shader::essl = savedEssl;
	Shader::glslVersion = savedVersion;
	EXPECT_TRUE(core::string::contains(out, "noperspective")) << out.c_str();
	EXPECT_FALSE(core::string::contains(out, " es\n")) << out.c_str();
}

TEST_F(ShaderTest, testGetSourcePragmaConstantBecomesDefine) {
	const bool savedEssl = Shader::essl;
	const int savedVersion = Shader::glslVersion;
	Shader::essl = false;
	Shader::glslVersion = 430;
	Shader s;
	const core::String out = s.getSource(ShaderType::Vertex, "#pragma constant FilterSize 3\n", true);
	Shader::essl = savedEssl;
	Shader::glslVersion = savedVersion;
	EXPECT_TRUE(core::string::contains(out, "#define FilterSize 3")) << out.c_str();
	EXPECT_FALSE(core::string::contains(out, "#pragma constant")) << out.c_str();
}

TEST_F(ShaderTest, testGetSourceEsslIntegerSamplerPrecision) {
	const bool savedEssl = Shader::essl;
	const int savedVersion = Shader::glslVersion;
	Shader::essl = true;
	Shader::glslVersion = 300;
	Shader s;
	const core::String out = s.getSource(ShaderType::Fragment, "void main() {}\n", true);
	Shader::essl = savedEssl;
	Shader::glslVersion = savedVersion;
	EXPECT_TRUE(core::string::contains(out, "precision highp usampler2D;")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "precision highp usampler3D;")) << out.c_str();
	EXPECT_TRUE(core::string::contains(out, "precision highp usampler2DArray;")) << out.c_str();
}

} // namespace video
