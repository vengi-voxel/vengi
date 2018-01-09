/**
 * @file
 */

#include "video/tests/AbstractGLTest.h"
#include "TurbobadgerShaders.h"

namespace ui {
namespace turbobadger {

class UIShaderTest : public video::AbstractGLTest {
};

TEST_F(UIShaderTest, testTextureShader) {
	if (!_supported) {
		return;
	}
	shader::TextureShader shader;
	ASSERT_TRUE(shader.setup());
	shader.shutdown();
}

}
}
