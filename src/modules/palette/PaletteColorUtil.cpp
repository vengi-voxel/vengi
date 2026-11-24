/**
 * @file
 */

#include "palette/PaletteColorUtil.h"
#include "color/ColorUtil.h"
#include "core/StandardLib.h"

namespace palette {

math::Vec4f color4(const Palette &palette, uint8_t paletteColorIdx) {
	return color::fromRGBA(palette.color(paletteColorIdx));
}

void toVec4f(const Palette &palette, math::Vec4f *materialColors, math::Vec4f *emitColors) {
	core_memset(materialColors, 0, sizeof(math::Vec4f) * PaletteMaxColors);
	core_memset(emitColors, 0, sizeof(math::Vec4f) * PaletteMaxColors);
	constexpr float scale = 1.0f / 255.0f;
	const int colorCount = palette.colorCount();
	for (int i = 0; i < colorCount; ++i) {
		const color::RGBA rgba = palette.color((uint8_t)i);
		const float r = (float)rgba.r * scale;
		const float g = (float)rgba.g * scale;
		const float b = (float)rgba.b * scale;
		const float a = (float)rgba.a * scale;
		materialColors[i] = math::Vec4f(r, g, b, a);
		if (palette.hasEmit((uint8_t)i)) {
			const float emit = palette.material((uint8_t)i).emit;
			emitColors[i] = math::Vec4f(r * emit, g * emit, b * emit, a * emit);
		}
	}
}

} // namespace palette
