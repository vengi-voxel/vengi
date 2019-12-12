/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "RenderShaders.h"

namespace render {

class RenderShaderTest : public video::AbstractGLTest {
};

TEST_F(RenderShaderTest, testTextureShader) {
	if (!_supported) {
		return;
	}
	shader::TextureShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(RenderShaderTest, testColorShader) {
	if (!_supported) {
		return;
	}
	shader::ColorShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

}
