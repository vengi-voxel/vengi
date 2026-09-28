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

}
