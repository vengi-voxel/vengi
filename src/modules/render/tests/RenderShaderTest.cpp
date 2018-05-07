/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "RenderShaders.h"

namespace render {

class RenderShaderTest : public video::AbstractGLTest {
};

TEST_F(RenderShaderTest, testMeshShader) {
	if (!_supported) {
		return;
	}
	shader::MeshShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(RenderShaderTest, testTextureShader) {
	if (!_supported) {
		return;
	}
	shader::TextureShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(RenderShaderTest, testColorShader) {
	if (!_supported) {
		return;
	}
	shader::ColorShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(RenderShaderTest, testComputeNoiseShader) {
	if (!_supported) {
		return;
	}
	shader::NoiseShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
