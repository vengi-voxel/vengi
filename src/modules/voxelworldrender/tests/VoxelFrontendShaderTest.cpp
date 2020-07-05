/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelworldrenderShaders.h"

namespace voxelworldrender {

class VoxelFrontendShaderTest : public video::AbstractGLTest, public ::testing::WithParamInterface<video::ShaderVarState> {
};

TEST_P(VoxelFrontendShaderTest, testWorldShader) {
	if (!_supported) {
		return;
	}
	const video::ShaderVarState val = GetParam();
	core::Var::get(cfg::ClientFog, "", core::CV_SHADER)->setVal(val.clientFog);
	core::Var::get(cfg::ClientShadowMap, "", core::CV_SHADER)->setVal(val.clientShadowMap);
	core::Var::get(cfg::ClientWater, "", core::CV_SHADER)->setVal(val.clientWater);
	core::Var::get(cfg::ClientDebugShadow, "", core::CV_SHADER)->setVal(val.clientDebugShadow);

	shader::WorldShader shader;
	EXPECT_TRUE(shader.setup());
	shader.shutdown();
}

INSTANTIATE_TEST_SUITE_P(
	ShaderVars,
	VoxelFrontendShaderTest,
	::testing::Values(
		video::ShaderVarState{true, true, true, true}
	)
);

}
