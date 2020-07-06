/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "RenderShaders.h"

namespace render {


class RenderShaderTest : public video::AbstractShaderTest {
};

TEST_P(RenderShaderTest, testTextureShader) {
	if (!_supported) {
		return;
	}
	shader::TextureShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(RenderShaderTest, testColorShader) {
	if (!_supported) {
		return;
	}
	shader::ColorShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

VIDEO_SHADERTEST(RenderShaderTest)

}
