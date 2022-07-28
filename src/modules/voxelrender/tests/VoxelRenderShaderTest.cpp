/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelrenderShaders.h"

namespace voxelworldrender {

class VoxelRenderShaderTest : public video::AbstractShaderTest {
};

TEST_P(VoxelRenderShaderTest, testVoxelShader) {
	if (!_supported) {
		return;
	}
	shader::VoxelShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testVoxelIndirectShader) {
	if (!_supported) {
		return;
	}
	shader::VoxelIndirectShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelRenderShaderTest, testVoxelInstancedShader) {
	if (!_supported) {
		return;
	}
	shader::VoxelInstancedShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

VIDEO_SHADERTEST(VoxelRenderShaderTest)

}
