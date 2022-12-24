/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "core/RGBA.h"
#include <stdint.h>
#include <glm/vec4.hpp>

namespace voxel {

static constexpr int PaletteMaxColors = 256;
// RGBA color values in the range [0-255]
using PaletteColorArray = core::RGBA[PaletteMaxColors];

class Palette {
private:
	bool load(const image::ImagePtr &img);
	bool _dirty = false;
	bool _needsSave = false;
	core::String _name;
	union hash {
		uint32_t _hashColors[2];
		uint64_t _hash;
	} _hash {};

	bool load(const uint8_t *rgbaBuf, size_t bufsize, const char *name);
public:
	PaletteColorArray colors {};
	PaletteColorArray glowColors {};
	int colorCount = 0;

	const core::String &name() const {
		return _name;
	}

	inline uint64_t hash() const {
		return _hash._hash;
	}
	inline size_t size() const {
		return colorCount;
	}
	bool load(const char *name);
	bool save(const char *name = nullptr) const;

	bool loadGimpPalette(const char *filename);
	bool loadRGBPalette(const char *filename);
	bool loadQubiclePalette(const char *filename);
	bool loadCSVPalette(const char *filename);

	bool saveGimpPalette(const char *filename, const char *name = "Noname") const;
	bool saveRGBPalette(const char *filename) const;
	bool saveCSVPalette(const char *filename) const;
	bool saveGlow(const char *name = nullptr) const;

	void changeIntensity(float scale);
	void reduce(uint8_t targetColors, core::Color::ColorReductionType reductionType = core::Color::ColorReductionType::Wu);

	static core::String print(const Palette &palette, bool colorAsHex = false);

	/**
	 * @brief fill the remaining colors with black
	 */
	void fill();

	static constexpr const char *builtIn[] = {"built-in:nippon", "built-in:minecraft", "built-in:magicavoxel",
											  "built-in:quake1", "built-in:commandandconquer"};

	bool minecraft();
	bool magicaVoxel();
	bool nippon();
	bool quake1();
	bool commandAndConquer();

	void markDirty();
	inline bool isDirty() const {
		return _dirty;
	}
	inline void markClean() {
		_dirty = false;
	}

	inline void markSave() {
		_needsSave = true;
	}
	inline bool needsSave() const {
		return _needsSave;
	}
	inline void markSaved() {
		_needsSave = false;
	}

	bool hasGlow(uint8_t idx) const;
	void removeGlow(uint8_t idx);
	void setGlow(uint8_t idx, float factor = 1.0f);

	/**
	 * @param rgba Normalized color value [0.0-1.0]
	 * @param distance Optional parameter to get the calculated distance for the selected color entry
	 * @return int The index to the palette color
	 */
	int getClosestMatch(const core::RGBA rgba, float *distance = nullptr, int skip = -1) const;
	uint8_t findReplacement(uint8_t index) const;
	/**
	 * @brief Will add the given color to the palette - and if the max colors are reached it will try
	 * to match the color to another already existing color in the palette.
	 * @note Only use this for single colors - not for a lot of them. This method is quite slow
	 * @param[in] skipSlotIndex This slot is not filled with any color value - if it's @c -1 every slot is filled
	 */
	bool addColorToPalette(core::RGBA rgba, bool skipSimilar = true, uint8_t *index = nullptr, bool replaceSimilar = true, int skipSlotIndex = -1);
	bool hasColor(core::RGBA rgba);
	void quantize(const core::RGBA *inputColors, const size_t inputColorCount);
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
		return builtIn[0];
	}

	static core::String extractPaletteName(const core::String& file);
	static bool createPalette(const image::ImagePtr& image, voxel::Palette &palette);
	static bool convertImageToPalettePng(const image::ImagePtr& image, const char *paletteFile);
};

} // namespace voxel
