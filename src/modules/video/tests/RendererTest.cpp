/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "video/Renderer.h"
#include "video/TextureConfig.h"

namespace video {

class RendererTest : public app::AbstractTest {
};

TEST_F(RendererTest, testMapType) {
	ASSERT_EQ(DataType::Float, mapType<decltype(glm::vec3::x)>());
}

TEST_F(RendererTest, testMapTypeStructOffset) {
	struct Buf {
		unsigned char c;
		int8_t b;
		uint8_t ub;
		int32_t i;
		uint32_t ui;
		int16_t s;
		uint16_t us;
	};
	ASSERT_EQ(DataType::UnsignedByte, mapType<decltype(Buf::c)>());
	ASSERT_EQ(DataType::Byte, mapType<decltype(Buf::b)>());
	ASSERT_EQ(DataType::UnsignedByte, mapType<decltype(Buf::ub)>());
	ASSERT_EQ(DataType::Int, mapType<decltype(Buf::i)>());
	ASSERT_EQ(DataType::UnsignedInt, mapType<decltype(Buf::ui)>());
	ASSERT_EQ(DataType::Short, mapType<decltype(Buf::s)>());
	ASSERT_EQ(DataType::UnsignedShort, mapType<decltype(Buf::us)>());
}

TEST_F(RendererTest, testFramebufferUvUIncreasesLeftToRight) {
	const glm::vec4 &uv = framebufferUV();
	EXPECT_LT(uv.x, uv.z);
}

TEST_F(RendererTest, testFramebufferUvYFlippedWhenClipOriginLowerLeft) {
	const glm::vec4 &uv = framebufferUV();
	if (clipOriginLowerLeft()) {
		EXPECT_GT(uv.y, uv.w);
	} else {
		EXPECT_LT(uv.y, uv.w);
	}
}

TEST_F(RendererTest, testIntegerTextureFormat) {
	EXPECT_TRUE(isIntegerTextureFormat(TextureFormat::R8U));
	EXPECT_TRUE(isIntegerTextureFormat(TextureFormat::R16U));
	EXPECT_TRUE(isIntegerTextureFormat(TextureFormat::R32U));
	EXPECT_TRUE(isIntegerTextureFormat(TextureFormat::RG16U));
	EXPECT_FALSE(isIntegerTextureFormat(TextureFormat::RGBA));
	EXPECT_FALSE(isIntegerTextureFormat(TextureFormat::RGBA16F));
}

TEST_F(RendererTest, testIntegerTextureConfigForcesNearest) {
	TextureConfig cfg;
	cfg.filter(TextureFilter::Linear);
	cfg.format(TextureFormat::R8U);
	EXPECT_EQ(TextureFilter::Nearest, cfg.filterMag());
	EXPECT_EQ(TextureFilter::Nearest, cfg.filterMin());
	EXPECT_FLOAT_EQ(0.0f, cfg.maxAnisotropy());

	cfg.filter(TextureFilter::LinearMipmapLinear);
	EXPECT_EQ(TextureFilter::Nearest, cfg.filterMag());
	EXPECT_EQ(TextureFilter::Nearest, cfg.filterMin());

	cfg.maxAnisotropy(16.0f);
	EXPECT_FLOAT_EQ(0.0f, cfg.maxAnisotropy());
}

}
