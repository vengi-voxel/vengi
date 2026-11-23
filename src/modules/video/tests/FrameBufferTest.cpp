/**
 * @file
 */

#include "color/RGBA.h"
#include "image/Image.h"
#include "video/FrameBuffer.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/TextureConfig.h"
#include "video/tests/AbstractGLTest.h"
#include "math/Vec4f.h"
#include "math/Vec2i.h"

namespace video {

class FrameBufferTest : public AbstractGLTest {};

static void restoreColorDrawBuffers(FrameBuffer &fb) {
	const Id prev = bindFramebuffer(fb.handle());
	const FrameBufferAttachment color01[] = {FrameBufferAttachment::Color0, FrameBufferAttachment::Color1};
	drawBuffers(2, color01);
	bindFramebuffer(prev);
}

static void fillAttachment(FrameBuffer &fb, FrameBufferAttachment attachment, const math::Vec4f &color) {
	fb.bind(false);
	drawBuffers(1, &attachment);
	clearColor(color);
	clear(ClearFlag::Color);
	restoreColorDrawBuffers(fb);
	fb.unbind();
}

static FrameBufferConfig twoColorConfig(const math::Vec2i &size) {
	FrameBufferConfig cfg;
	cfg.dimension(size);
	cfg.addTextureAttachment(createDefaultTextureConfig(), FrameBufferAttachment::Color0);
	cfg.addTextureAttachment(createDefaultTextureConfig(), FrameBufferAttachment::Color1);
	return cfg;
}

TEST_F(FrameBufferTest, testBlitColorAttachmentDoesNotCopyColor0OntoColor1) {
	if (IsSkipped()) {
		return;
	}
	if (isVulkanRenderer()) {
		GTEST_SKIP() << "Vulkan FBO blit/readback still needs an active frame path in this test harness";
	}
	const math::Vec2i size(8, 8);
	FrameBuffer src;
	FrameBuffer dst;
	ASSERT_TRUE(src.init(twoColorConfig(size)));
	ASSERT_TRUE(dst.init(twoColorConfig(size)));

	fillAttachment(src, FrameBufferAttachment::Color0, math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	fillAttachment(src, FrameBufferAttachment::Color1, math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	fillAttachment(dst, FrameBufferAttachment::Color0, math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
	fillAttachment(dst, FrameBufferAttachment::Color1, math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	blitFramebuffer(src.handle(), dst.handle(), FrameBufferAttachment::Color0, size.x, size.y);
	blitFramebuffer(src.handle(), dst.handle(), FrameBufferAttachment::Color1, size.x, size.y);
	restoreColorDrawBuffers(dst);
	restoreColorDrawBuffers(src);

	const image::ImagePtr color0 = dst.image("color0", FrameBufferAttachment::Color0);
	const image::ImagePtr color1 = dst.image("color1", FrameBufferAttachment::Color1);
	ASSERT_TRUE(color0);
	ASSERT_TRUE(color1);
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color0->colorAt(0, 0));
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), color1->colorAt(0, 0));

	src.shutdown();
	dst.shutdown();
}

TEST_F(FrameBufferTest, testColorFlagBlitLeavesColor1Unchanged) {
	if (IsSkipped()) {
		return;
	}
	if (isVulkanRenderer()) {
		GTEST_SKIP() << "Vulkan FBO blit/readback still needs an active frame path in this test harness";
	}
	const math::Vec2i size(8, 8);
	FrameBuffer src;
	FrameBuffer dst;
	ASSERT_TRUE(src.init(twoColorConfig(size)));
	ASSERT_TRUE(dst.init(twoColorConfig(size)));

	fillAttachment(src, FrameBufferAttachment::Color0, math::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	fillAttachment(src, FrameBufferAttachment::Color1, math::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	fillAttachment(dst, FrameBufferAttachment::Color0, math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
	fillAttachment(dst, FrameBufferAttachment::Color1, math::Vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	blitFramebuffer(src.handle(), dst.handle(), ClearFlag::Color, size.x, size.y);
	restoreColorDrawBuffers(dst);
	restoreColorDrawBuffers(src);

	const image::ImagePtr color0 = dst.image("color0", FrameBufferAttachment::Color0);
	const image::ImagePtr color1 = dst.image("color1", FrameBufferAttachment::Color1);
	ASSERT_TRUE(color0);
	ASSERT_TRUE(color1);
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color0->colorAt(0, 0));
	EXPECT_EQ(color::RGBA(0, 0, 255, 255), color1->colorAt(0, 0));

	src.shutdown();
	dst.shutdown();
}

} // namespace video
