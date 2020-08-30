/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "video/Shader.h"
#include "core/io/Filesystem.h"
#include "core/StringUtil.h"
#include "core/Var.h"

namespace video {

class ShaderTest : public core::AbstractTest {
};

TEST_F(ShaderTest, testInclude) {
	const io::FilesystemPtr& filesystem = _testApp->filesystem();

	filesystem->write("foobar.vert", "#define SUCCESS");
	filesystem->write("foobar.frag", "#define SUCCESS");

	Shader s;
	const core::String &vert = s.getSource(ShaderType::Vertex, "#include \"foobar.vert\"");
	const core::String &frag = s.getSource(ShaderType::Fragment, "#include \"foobar.frag\"");
	ASSERT_TRUE(core::string::contains(vert, "SUCCESS")) << "vertex shader: " << vert;
	ASSERT_TRUE(core::string::contains(frag, "SUCCESS")) << "fragment shader: " << frag;
}

TEST_F(ShaderTest, testCvar) {
	const core::VarPtr& v = core::Var::get("awesome_name", "true", core::CV_SHADER);
	ASSERT_EQ(core::CV_SHADER, v->getFlags() & core::CV_SHADER);
	ASSERT_EQ("true", v->strVal());
	Shader s;
	const core::String &vert = s.getSource(ShaderType::Vertex, "#define FOO");
	const core::String &name = Shader::validPreprocessorName(v->name());
	ASSERT_TRUE(core::string::contains(vert, name)) << "vertex shader: " << vert;
}

}
