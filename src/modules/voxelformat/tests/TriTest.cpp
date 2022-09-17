/**
 * @file
 */

#include "voxelformat/private/Tri.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "image/Image.h"
#include "voxel/Palette.h"

namespace voxelformat {

class TriTest : public app::AbstractTest {};

TEST_F(TriTest, testMinsMaxs) {
	Tri tri;
	tri.vertices[0] = glm::vec3(-20, -10, -23);
	tri.vertices[1] = glm::vec3(-10, -30, 23);
	tri.vertices[2] = glm::vec3(20, 30, 40);
	const glm::vec3 mins = tri.mins();
	const glm::vec3 maxs = tri.maxs();
	EXPECT_FLOAT_EQ(-20.0f, mins.x);
	EXPECT_FLOAT_EQ(-30.0f, mins.y);
	EXPECT_FLOAT_EQ(-23.0f, mins.z);
	EXPECT_FLOAT_EQ(20.0f, maxs.x);
	EXPECT_FLOAT_EQ(30.0f, maxs.y);
	EXPECT_FLOAT_EQ(40.0f, maxs.z);
}

TEST_F(TriTest, testFlat) {
	Tri tri;
	tri.vertices[0] = glm::vec3(0, 0, 0);
	tri.vertices[1] = glm::vec3(1, 0, 0);
	tri.vertices[2] = glm::vec3(0, 0, 1);
	EXPECT_TRUE(tri.flat()) << tri.normal();
	tri.vertices[0] = glm::vec3(0, 0, 0);
	tri.vertices[1] = glm::vec3(1, 1, 0);
	tri.vertices[2] = glm::vec3(0, 0, 1);
	EXPECT_FALSE(tri.flat()) << tri.normal();
}

TEST_F(TriTest, testColorAt) {
	const image::ImagePtr &texture = image::loadImage("palette-nippon.png", false);
	ASSERT_TRUE(texture);
	ASSERT_EQ(256, texture->width());
	ASSERT_EQ(1, texture->height());

	voxel::Palette pal;
	pal.nippon();

	Tri tri;
	tri.texture = texture.get();
	for (int i = 0; i < 256; ++i) {
		tri.uv[0] = glm::vec2((float)i / 256.0f, 0.0f);
		tri.uv[1] = glm::vec2((float)i / 256.0f, 1.0f);
		tri.uv[2] = glm::vec2((float)(i + 1) / 256.0f, 1.0f);
		const core::RGBA color = tri.colorAt(tri.centerUV());
		ASSERT_EQ(pal.colors[i], color) << "i: " << i << " " << core::Color::print(pal.colors[i]) << " vs "
										<< core::Color::print(color);
	}
}

TEST_F(TriTest, testColorAt4x4) {
	constexpr int h = 4;
	constexpr int w = 4;
	constexpr core::RGBA buffer[w * h]{
		{255, 0, 0, 255},	{255, 255, 0, 255},	  {255, 0, 255, 255},	{255, 255, 255, 255},
		{0, 255, 0, 255},	{13, 255, 50, 255},	  {127, 127, 127, 255}, {255, 127, 0, 255},
		{255, 0, 0, 255},	{255, 60, 0, 255},	  {255, 0, 30, 255},	{127, 69, 255, 255},
		{127, 127, 0, 255}, {255, 127, 127, 255}, {255, 0, 127, 255},	{0, 127, 80, 255}};
	static_assert(sizeof(buffer) == (size_t)w * (size_t)h * sizeof(uint32_t), "Unexpected rgba buffer size");
	const image::ImagePtr &texture = image::createEmptyImage("4x4");
	texture->loadRGBA((const uint8_t *)buffer, w, h);
	ASSERT_TRUE(texture);
	ASSERT_EQ(w, texture->width());
	ASSERT_EQ(h, texture->height());

	Tri tri;
	tri.texture = texture.get();
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			const float epsilon = 0.0001f;
			tri.uv[0] = glm::vec2((float)i / (float)w, (float)j / (float)h);
			tri.uv[1] = glm::vec2((float)i / (float)w, (float)(j + 1) / (float)h - epsilon);
			tri.uv[2] = glm::vec2((float)(i + 1) / (float)w - epsilon, (float)(j + 1) / (float)h - epsilon);
			const core::RGBA color = tri.colorAt(tri.centerUV());
			const int texIndex = j * w + i;
			ASSERT_EQ(buffer[texIndex], color) << "i: " << i << "/" << j << " " << core::Color::print(buffer[texIndex])
											   << " vs " << core::Color::print(color) << " ti: " << texIndex;
		}
	}
}

} // namespace voxelformat
