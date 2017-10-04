/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "UiShaders.h"

namespace frontend {

class UIShaderTest : public video::AbstractGLTest {
};

TEST_F(UIShaderTest, testTextureShader) {
	if (!_supported) {
		return;
	}
	shader::TextureShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
