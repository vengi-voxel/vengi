/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "video/Renderer.h"
#include "video/TextureConfig.h"
#include "video/Buffer.h"
#include "video/UploadRing.h"
#include "video/tests/AbstractGLTest.h"

namespace video {

class RendererTest : public app::AbstractTest {};

class RendererStreamingTest : public AbstractGLTest {};
TEST_F(RendererStreamingTest, EmptyInitialDataStillAllocatesWritableStorage) {
	Buffer buffer;
	const int index = buffer.create(nullptr, 16);
	ASSERT_GE(index, 0);
	auto *data = (uint32_t *)mapBuffer(buffer.bufferHandle(index), BufferType::ArrayBuffer, AccessMode::Write);
	ASSERT_NE(nullptr, data);
	data[0] = 1741;
	data[3] = 2701;
	ASSERT_TRUE(unmapBuffer(buffer.bufferHandle(index), BufferType::ArrayBuffer));
	data = (uint32_t *)mapBuffer(buffer.bufferHandle(index), BufferType::ArrayBuffer, AccessMode::Read);
	ASSERT_NE(nullptr, data);
	EXPECT_EQ(1741u, data[0]);
	EXPECT_EQ(2701u, data[3]);
	ASSERT_TRUE(unmapBuffer(buffer.bufferHandle(index), BufferType::ArrayBuffer));
	buffer.shutdown();
}
TEST_F(RendererStreamingTest, CompletionFencesCanBeQueriedAndReleased) {
	Fence fence = createFence();
	if (!fence)
		GTEST_SKIP() << "Completion fences unavailable";
	video::finish();
	EXPECT_TRUE(fenceSignaled(fence));
	deleteFence(fence);
	EXPECT_EQ(nullptr, fence);
	EXPECT_TRUE(fenceSignaled(nullptr));
}

TEST_F(RendererStreamingTest, QueuedUploadsPreserveEveryFrameAcrossRingReuseAndOrphaning) {
	if (!uploadRingEnabled())
		GTEST_SKIP() << "Persistent upload ring unavailable";
	constexpr int frames = 12, words = 65536;
	alignas(16) uint32_t payload[words];
	for (bool orphaning : {false, true}) {
		Buffer source;
		const int index = source.create();
		source.setMode(index, BufferMode::Stream);
		ASSERT_TRUE(source.update(index, nullptr, sizeof(payload), orphaning));
		Id snapshots[frames];
		for (int frame = 0; frame < frames; ++frame) {
			for (int word = 0; word < words; ++word)
				payload[word] = (uint32_t)(frame * words + word);
			uploadRingBeginFrame();
			ASSERT_TRUE(source.update(index, payload, sizeof(payload), orphaning));
			snapshots[frame] = genBuffer();
			bufferData(snapshots[frame], BufferType::ArrayBuffer, BufferMode::Static, nullptr, sizeof(payload));
			ASSERT_TRUE(copyBufferSubData(source.bufferHandle(index), 0, snapshots[frame], 0, sizeof(payload)));
			uploadRingEndFrame();
		}
		video::finish();
		for (int frame = 0; frame < frames; ++frame) {
			const auto *data = (const uint32_t *)mapBuffer(snapshots[frame], BufferType::ArrayBuffer, AccessMode::Read);
			ASSERT_NE(nullptr, data);
			for (int word = 0; word < words; word += 17)
				EXPECT_EQ((uint32_t)(frame * words + word), data[word]);
			ASSERT_TRUE(unmapBuffer(snapshots[frame], BufferType::ArrayBuffer));
			deleteBuffer(snapshots[frame]);
		}
		source.shutdown();
	}
}

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

} // namespace video
