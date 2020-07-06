/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelworldrenderShaders.h"

namespace voxelworldrender {

class VoxelFrontendShaderTest : public video::AbstractShaderTest {
};

TEST_P(VoxelFrontendShaderTest, testWorldShader) {
	if (!_supported) {
		return;
	}
	shader::WorldShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelFrontendShaderTest, testWaterShader) {
	if (!_supported) {
		return;
	}
	shader::WaterShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

VIDEO_SHADERTEST(VoxelFrontendShaderTest)

}
