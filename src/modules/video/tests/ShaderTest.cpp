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
	const std::string &vert = s.getSource(SHADER_VERTEX, "#include \"foobar.vert\"");
	const std::string &frag = s.getSource(SHADER_FRAGMENT, "#include \"foobar.frag\"");
	ASSERT_TRUE(core::string::contains(vert, "SUCCESS"));
	ASSERT_TRUE(core::string::contains(frag, "SUCCESS"));
}

}
