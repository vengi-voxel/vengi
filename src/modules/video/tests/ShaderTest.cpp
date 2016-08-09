/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "video/Shader.h"

namespace video {

class ShaderTest : public core::AbstractTest {

};

TEST_F(ShaderTest, testInclude) {
	io::FilesystemPtr filesystem = core::App::getInstance()->filesystem();

	filesystem->write("foobar.vert", "#define SUCCESS");
	filesystem->write("foobar.frag", "#define SUCCESS");

	Shader s;
	const std::string &vert = s.getSource(ShaderType::Vertex, "#include \"foobar.vert\"");
	const std::string &frag = s.getSource(ShaderType::Fragment, "#include \"foobar.frag\"");
	ASSERT_TRUE(core::string::contains(vert, "SUCCESS"));
	ASSERT_TRUE(core::string::contains(frag, "SUCCESS"));
}

TEST_F(ShaderTest, testCvar) {
	core::Var::get("awesome_name", "true", core::CV_SHADER);
	Shader s;
	const std::string &vert = s.getSource(ShaderType::Vertex, "");
	ASSERT_TRUE(core::string::contains(vert, "awesome_name"));
}

}
