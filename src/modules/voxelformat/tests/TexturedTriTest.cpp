/**
 * @file
 */

#include "voxelformat/private/mesh/TexturedTri.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "image/Image.h"

namespace voxelformat {

class TexturedTriTest : public app::AbstractTest {};

TEST_F(TexturedTriTest, testColorAt4x4) {
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

	voxelformat::TexturedTri tri;
	tri.texture = texture;
	for (int i = 0; i < w; ++i) {
		for (int j = 0; j < h; ++j) {
			tri.uv[0] = image::Image::uv(i, j, w, h);
			tri.uv[1] = image::Image::uv(i, j + 1, w, h);
			tri.uv[2] = image::Image::uv(i + 1, j, w, h);
			const core::RGBA color = tri.colorAt(tri.centerUV());
			const int texIndex = j * w + i;
			ASSERT_EQ(buffer[texIndex], color) << "i: " << i << "/" << j << " " << core::Color::print(buffer[texIndex])
											   << " vs " << core::Color::print(color) << " ti: " << texIndex;
		}
	}
}

} // namespace voxelformat
