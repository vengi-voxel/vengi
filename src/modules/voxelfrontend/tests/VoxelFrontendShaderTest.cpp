/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "VoxelfrontendShaders.h"

namespace voxelfrontend {

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
