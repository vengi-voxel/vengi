/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "ColorShader.h"
#include "TextureShader.h"
namespace render {


class RenderShaderTest : public video::AbstractShaderTest {
};

TEST_P(RenderShaderTest, testTextureShader) {
	shader::TextureShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(RenderShaderTest, testColorShader) {
	shader::ColorShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

VIDEO_SHADERTEST(RenderShaderTest)

}
