/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelShader.h"
#include "VoxelnormShader.h"
#include "VoxeloitShader.h"
#include "VoxelnormoitShader.h"
#include "OitShader.h"

namespace voxelrender {

class VoxelRenderShaderTest : public video::AbstractShaderTest {
};

TEST_P(VoxelRenderShaderTest, testVoxelShader) {
	shader::VoxelShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testVoxelNormShader) {
	shader::VoxelnormShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testVoxelOitShader) {
	shader::VoxeloitShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testVoxelNormOitShader) {
	shader::VoxelnormoitShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testOitShader) {
	shader::OitShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

VIDEO_SHADERTEST(VoxelRenderShaderTest)

}
