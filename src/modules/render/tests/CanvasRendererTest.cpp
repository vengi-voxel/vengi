/**
 * @file
 */
#include "render/CanvasRenderer.h"
#include "image/Image.h"
#include "video/FrameBuffer.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/tests/AbstractGLTest.h"
namespace render {
class CanvasRendererTest : public video::AbstractGLTest {};
TEST_F(CanvasRendererTest, NativeCanvasClipsBlendsAndRestoresScissorOnHighDpiTargets) {
	CanvasRenderer canvas;
	ASSERT_TRUE(canvas.init());
	video::FrameBuffer target;
	video::FrameBufferConfig config;
	config.dimension(glm::ivec2(128)).colorTexture(true);
	ASSERT_TRUE(target.init(config));
	video::clearColor(glm::vec4(0, 0, 0, 1));
	target.bind(true);
	canvas.begin(glm::vec2(64));
	canvas.rect(glm::vec4(0, 0, 64, 64), glm::vec4(1, 0, 0, 1), 4);
	canvas.clip(glm::vec4(16, 16, 16, 16));
	canvas.rect(glm::vec4(0, 0, 64, 64), glm::vec4(0, 0, 1, 0.5f));
	ASSERT_TRUE(canvas.pushClip(glm::vec4(22, 22, 40, 40)));
	canvas.rect(glm::vec4(0, 0, 64, 64), glm::vec4(0, 1, 0, 1));
	ASSERT_TRUE(canvas.popClip());
	EXPECT_EQ(glm::vec4(16, 16, 16, 16), canvas.clipRect());
	EXPECT_FALSE(canvas.popClip());
	canvas.resetClip();
	canvas.text(glm::vec2(2, 42), "GPU", glm::vec4(1), 16);
	video::scissor(1, 2, 3, 4);
	const uint64_t draws = video::renderStats().drawCalls;
	canvas.render();
	EXPECT_EQ(1u, video::renderStats().drawCalls - draws);
	int x, y, w, h;
	video::getScissor(x, y, w, h);
	EXPECT_EQ(1, x);
	EXPECT_EQ(2, y);
	EXPECT_EQ(3, w);
	EXPECT_EQ(4, h);
	uint8_t *pixels = nullptr;
	ASSERT_TRUE(video::readFramebuffer(0, 0, 128, 128, &pixels));
	image::Image::flipVerticalRGBA(pixels, 128, 128);
	auto result = image::createEmptyImage("canvas");
	ASSERT_TRUE(result->loadRGBA(pixels, 128, 128));
	core_free(pixels);
	const auto inside = result->colorAt(40, 40);
	const auto outside = result->colorAt(80, 40);
	EXPECT_GT(inside.b, 100);
	EXPECT_LT(inside.r, 160);
	EXPECT_GT(outside.r, 240);
	EXPECT_EQ(0, outside.b);
	EXPECT_GT(result->colorAt(48, 48).g, 240);
	target.unbind();
	canvas.shutdown();
	target.shutdown();
}
} // namespace render
