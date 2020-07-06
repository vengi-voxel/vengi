/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelworldrenderShaders.h"

namespace voxelworldrender {

class VoxelFrontendShaderTest :
		public video::AbstractGLTest,
		public ::testing::WithParamInterface<video::ShaderVarState> {
};

TEST_P(VoxelFrontendShaderTest, testWorldShader) {
	if (!_supported) {
		return;
	}
	setShaderVars(GetParam());
	shader::WorldShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

TEST_P(VoxelFrontendShaderTest, testWaterShader) {
	if (!_supported) {
		return;
	}
	setShaderVars(GetParam());
	shader::WaterShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

INSTANTIATE_TEST_SUITE_P(
	ShaderVars,
	VoxelFrontendShaderTest,
	::testing::Values(
		video::ShaderVarState{true, true, true, true},
		video::ShaderVarState{true, true, false, false},
		video::ShaderVarState{true, true, true, false},
		video::ShaderVarState{true, false, false, false},
		video::ShaderVarState{true, false, false, true},
		video::ShaderVarState{true, false, true, true},
		video::ShaderVarState{false, false, false, false},
		video::ShaderVarState{false, true, false, false},
		video::ShaderVarState{false, true, true, false},
		video::ShaderVarState{false, true, true, true},
		video::ShaderVarState{false, false, true, false},
		video::ShaderVarState{false, false, true, true},
		video::ShaderVarState{false, false, false, true}
	)
);

}
