/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelrenderShaders.h"

namespace voxelrender {

class VoxelFrontendShaderTest : public video::AbstractGLTest {
};

TEST_F(VoxelFrontendShaderTest, testWorldShader) {
	if (!_supported) {
		return;
	}
	shader::WorldShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
