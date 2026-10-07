/**
 * @file
 */

#include "voxelformat/private/mesh/MeshTri.h"
#include "app/tests/AbstractTest.h"
#include "color/ColorUtil.h"
#include "image/Image.h"
#include "voxelformat/private/mesh/MeshMaterial.h"

namespace voxelformat {

class MeshTriTest : public app::AbstractTest {};

TEST_F(MeshTriTest, testVertexColorInterpolation) {
	MeshTri tri;
	tri.setColor(color::RGBA(255, 0, 0, 255), color::RGBA(0, 255, 0, 128), color::RGBA(0, 0, 255, 0));
	MeshMaterialArray materials;
	const glm::vec3 weights[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0.25f, 0.25f, 0.5f}};
	const color::RGBA expected[] = {tri.color0(), tri.color1(), tri.color2(), {64, 64, 128, 96}};
	for (int i = 0; i < 4; ++i) {
		EXPECT_EQ(expected[i], colorAt(tri, materials, glm::vec2(0), false, &weights[i]));
	}
}

TEST_F(MeshTriTest, testColorAt4x4) {
	constexpr int h = 4;
	constexpr int w = 4;
	constexpr color::RGBA buffer[w * h]{
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
		voxelformat::MeshTri meshTri;
		MeshMaterialArray meshMaterialArray;
		meshMaterialArray.emplace_back(createMaterial(texture));
		meshTri.materialIdx = meshMaterialArray.size() - 1;
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				meshTri.setUVs(image::Image::uv(x, y, w, h, originUpperLeft),
							  image::Image::uv(x, y + 1, w, h, originUpperLeft),
							  image::Image::uv(x + 1, y, w, h, originUpperLeft));
				const glm::vec2 &uv = meshTri.centerUV();
				const glm::vec3 weights(1.0f, 0.0f, 0.0f);
				const color::RGBA color = colorAt(meshTri, meshMaterialArray, uv, originUpperLeft, &weights);
				const int texIndex = y * w + x;
				ASSERT_EQ(buffer[texIndex], color)
					<< "pixel(" << x << "/" << y << "), " << color::print(buffer[texIndex]) << " vs "
					<< color::print(color) << " ti: " << texIndex;
			}
		}
	}
}

} // namespace voxelformat
