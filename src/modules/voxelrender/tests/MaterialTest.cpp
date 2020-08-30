/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "core/collection/DynamicArray.h"

namespace voxelrender {

class MaterialTest: public app::AbstractTest {
protected:
	const int components = 4;
	const int w = 256;
	const int h = 256;

	bool WriteImage(const char* name, uint8_t* buffer, int w = 256, int h = 256, int components = 4) {
		return image::Image::writePng(name, buffer, w, h, components);
	}
};

TEST_F(MaterialTest, testMaterial) {
	const int w = 256;
	const int h = 1024;
	core::DynamicArray<uint8_t> buffer(w * h * components);

	ASSERT_TRUE(voxel::initDefaultMaterialColors());
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const int amount = materialColors.size();
	ASSERT_GT(amount, 0) << "No material colors are defined";
	auto line_height = h / amount;

	int y = 0;
	for (int color_index = 0; color_index < amount; ++color_index) {
		auto line_y = 0;
		auto color = materialColors[color_index];
		for (; line_y < line_height; ++y, ++line_y) {
			for (int x = 0; x < w; ++x) {
				int index = y * (w * components) + (x * components);
				buffer[index++] = line_y == 0 ? 0 : color.r * 255;
				buffer[index++] = line_y == 0 ? 0 : color.g * 255;
				buffer[index++] = line_y == 0 ? 0 : color.b * 255;
				buffer[index++] = line_y == 0 ? 255 : color.a * 255;
			}
		}
	}
	ASSERT_TRUE(WriteImage("material.png", &buffer[0], w, h));
}

}
