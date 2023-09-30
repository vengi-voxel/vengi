/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelShader.h"
#include "VoxelnormShader.h"

namespace voxelworldrender {

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

VIDEO_SHADERTEST(VoxelRenderShaderTest)

}
