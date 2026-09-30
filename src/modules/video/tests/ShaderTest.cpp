/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "video/Shader.h"
#include "io/Filesystem.h"
#include "core/StringUtil.h"
#include "core/Var.h"

namespace video {

class ShaderTest : public app::AbstractTest {
};

TEST_F(ShaderTest, testInclude) {
	const io::FilesystemPtr& filesystem = _testApp->filesystem();

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
	const core::String out = s.getSource(ShaderType::Vertex, "noperspective $out float v_edge;\nflat $out float v_keep;\n", true);
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
	const core::String out = s.getSource(ShaderType::Vertex, "noperspective $out float v_edge;\n", true);
	Shader::essl = savedEssl;
	Shader::glslVersion = savedVersion;
	EXPECT_TRUE(core::string::contains(out, "noperspective")) << out.c_str();
	EXPECT_FALSE(core::string::contains(out, " es\n")) << out.c_str();
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

}
