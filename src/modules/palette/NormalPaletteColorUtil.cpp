/**
 * @file
 */

#include "palette/NormalPaletteColorUtil.h"
#include "core/StandardLib.h"

namespace palette {

void toVec4f(const NormalPalette &palette, math::Vec4f *vec4f) {
	core_memset(vec4f, 0, sizeof(math::Vec4f) * NormalPaletteMaxNormals);
	constexpr float scale = 2.0f / 255.0f;
	const int count = (int)palette.size();
	for (int i = 0; i < count; ++i) {
		const color::RGBA &rgba = palette.normal((uint8_t)i);
		if (rgba.a == 0) {
			continue;
		}
		vec4f[i].x = (float)rgba.r * scale - 1.0f;
		vec4f[i].y = (float)rgba.g * scale - 1.0f;
		vec4f[i].z = (float)rgba.b * scale - 1.0f;
	}
}

} // namespace palette
