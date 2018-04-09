/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "FrontendShaders.h"

namespace frontend {

class FrontendShaderTest : public video::AbstractGLTest {
};

TEST_F(FrontendShaderTest, testMeshShader) {
	if (!_supported) {
		return;
	}
	shader::MeshShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(FrontendShaderTest, testColorShader) {
	if (!_supported) {
		return;
	}
	shader::ColorShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(FrontendShaderTest, testComputeNoiseShader) {
	if (!_supported) {
		return;
	}
	shader::NoiseShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
