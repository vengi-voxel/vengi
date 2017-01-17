/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "FrontendShaders.h"

namespace frontend {

class FrontendShaderTest : public video::AbstractGLTest {
};

TEST_F(FrontendShaderTest, testWorldShader) {
	shader::WorldShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(FrontendShaderTest, testMeshShader) {
	shader::MeshShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_F(FrontendShaderTest, testColorShader) {
	shader::ColorShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
