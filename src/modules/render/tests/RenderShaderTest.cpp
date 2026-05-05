/**
 * @file
 */

#include "ColorShader.h"
#include "Combine2Shader.h"
#include "ConvolutionShader.h"
#include "TextureShader.h"
#include "video/tests/AbstractGLTest.h"
#include "../BloomRenderer.h"

namespace render {

class RenderShaderTest : public video::AbstractShaderTest {};

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

TEST_P(RenderShaderTest, testCombine2Shader) {
	shader::Combine2Shader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(RenderShaderTest, testConvolutionShader) {
	shader::ConvolutionShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(RenderShaderTest, testBloomRendererInit) {
	BloomRenderer bloom;
	ASSERT_TRUE(bloom.init(false, 64, 64));
	bloom.shutdown();
}

VIDEO_SHADERTEST(RenderShaderTest)

} // namespace render
