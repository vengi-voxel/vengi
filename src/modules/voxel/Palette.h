/**
 * @file
 */

#pragma once

#include "core/String.h"
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
	bool _dirty = false;
	bool _needsSave = false;
	core::String _paletteFilename;
public:
	PaletteColorArray colors {};
	PaletteColorArray glowColors {};
	int colorCount = 0;
	core::String lua;

	inline size_t size() const {
		return colorCount;
	}
	bool load(const char *name);
	bool save(const char *name = nullptr);
	bool load(const uint8_t *rgbaBuf, size_t bufsize);

	bool minecraft();
	bool magicaVoxel();

	void markDirty() {
		_dirty = true;
	}

	bool isDirty() const {
		return _dirty;
	}

	void markClean() {
		_dirty = false;
	}

	void markSave() {
		_needsSave = true;
	}
	bool needsSave() const {
		return _needsSave;
	}
	void markSaved() {
		_needsSave = false;
	}

	bool hasGlow(uint8_t idx) const;
	void removeGlow(uint8_t idx);
	void setGlow(uint8_t idx, float factor = 1.0f);

	/**
	 * @param color Normalized color value [0.0-1.0]
	 * @param distance Optional parameter to get the calculated distance for the selected color entry
	 * @return int The index to the palette color
	 */
	int getClosestMatch(const glm::vec4& color, float *distance = nullptr, int skip = -1) const;
	int getClosestMatch(const uint32_t rgba, float *distance = nullptr, int skip = -1) const;

	/**
	 * @brief Will add the given color to the palette - and if the max colors are reached it will try
	 * to remove a color that is most similar to another already existing color in the palette.
	 */
	bool addColorToPalette(uint32_t rgba, bool skipSimilar = true);

	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void toVec4f(core::DynamicArray<glm::vec4> &rgba) const;
	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void glowToVec4f(core::DynamicArray<glm::vec4> &vec4f) const;

	static const char* getDefaultPaletteName() {
		return "nippon";
	}

	static core::String extractPaletteName(const core::String& file);
	static bool createPalette(const image::ImagePtr& image, voxel::Palette &palette);
	static bool convertImageToPalettePng(const image::ImagePtr& image, const char *paletteFile);
};

} // namespace voxel
