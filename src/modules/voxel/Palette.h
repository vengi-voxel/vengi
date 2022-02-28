/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include <stdint.h>
#include <glm/vec4.hpp>

namespace voxel {

static constexpr int PaletteMaxColors = 256;
// RGBA color values in the range [0-255]
using PaletteColorArray = uint32_t[PaletteMaxColors];

class Palette {
private:
	bool load(const image::ImagePtr &img);

public:
	PaletteColorArray colors {};
	PaletteColorArray glowColors {};
	int colorCount = 0;
	core::String lua;

	inline size_t size() const {
		return colorCount;
	}
	bool load(const char *name);
	bool save(const char *name);
	bool load(const uint8_t *rgbaBuf, size_t bufsize);

	bool minecraft();
	bool magicaVoxel();

	bool hasGlow(uint8_t idx) const;
	void removeGlow(uint8_t idx);
	void setGlow(uint8_t idx, float factor = 1.0f);

	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 */
	void toVec4f(core::DynamicArray<glm::vec4> &rgba) const;
	void glowToVec4f(core::DynamicArray<glm::vec4> &vec4f) const;

	static const char* getDefaultPaletteName() {
		return "nippon";
	}

	static core::String extractPaletteName(const core::String& file);
	static bool createPalette(const image::ImagePtr& image, voxel::Palette &palette);
	static bool convertImageToPalettePng(const image::ImagePtr& image, const char *paletteFile);
};

} // namespace voxel
