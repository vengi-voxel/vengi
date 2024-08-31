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

	for (int s = 0; s < 2; ++s) {
		const bool originUpperLeft = s == 0;
		SCOPED_TRACE(s);
		voxelformat::TexturedTri tri;
		tri.texture = texture;
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				tri.uv[0] = image::Image::uv(x, y, w, h, originUpperLeft);
				tri.uv[1] = image::Image::uv(x, y + 1, w, h, originUpperLeft);
				tri.uv[2] = image::Image::uv(x + 1, y, w, h, originUpperLeft);
				const glm::vec2 &uv = tri.centerUV();
				const core::RGBA color = tri.colorAt(uv, originUpperLeft);
				const int texIndex = y * w + x;
				ASSERT_EQ(buffer[texIndex], color)
					<< "pixel(" << x << "/" << y << "), " << core::Color::print(buffer[texIndex]) << " vs "
					<< core::Color::print(color) << " ti: " << texIndex << ", uv(" << uv.x << "/" << uv.y
					<< ") triangle uvs(" << tri.uv[0].x << "/" << tri.uv[0].y << ", " << tri.uv[1].x << "/"
					<< tri.uv[1].y << ", " << tri.uv[2].x << "/" << tri.uv[2].y << ")";
			}
		}
	}
}

} // namespace voxelformat
