/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "color/ColorUtil.h"
#include "core/DirtyState.h"
#include "core/Optional.h"
#include "color/RGBA.h"
#include "core/String.h"
#include "core/collection/Array.h"
#include "core/collection/Buffer.h"
#include "palette/PaletteView.h"
#include "image/Image.h"
#include "palette/Material.h"
#include <glm/vec4.hpp>
#include <stdint.h>

namespace palette {

static const int PaletteColorNotFound = -1;

/**
 * @brief A 256 color palette
 */
class Palette : public core::DirtyState {
private:
	bool _needsSave = false;
	mutable bool _hashDirty = false;
	core::String _name;
	core::String _filename;
	mutable union hash {
		uint32_t _hashColors[2];
		uint64_t _hash;
	} _hash{};
	PaletteView _view;
	color::RGBA _colors[PaletteMaxColors]{};
	core::Optional<core::Array<core::String, PaletteMaxColors>> _names;
	Material _materials[PaletteMaxColors]{};
	int _colorCount = 0;

	bool load(const uint8_t *rgbaBuf, size_t bufsize, const char *name);
	int findInsignificant(int skipSlotIndex) const;
	bool loadLospec(const core::String &lospecId, const core::String &gimpPalette);

public:
	Palette();
	Palette(const Palette &other);
	Palette &operator=(const Palette &other);
	Palette(Palette &&other) noexcept;
	Palette &operator=(Palette &&other) noexcept;

	PaletteView &view();
	const PaletteView &view() const;

	void exchange(uint8_t paletteColorIdx1, uint8_t paletteColorIdx2);
	void copy(uint8_t fromPaletteColorIdx, uint8_t toPaletteColorIdx);

	const Material &material(uint8_t paletteColorIdx) const;
	color::RGBA color(uint8_t paletteColorIdx) const;
	glm::vec4 color4(uint8_t paletteColorIdx) const;
	void setColor(uint8_t paletteColorIdx, const color::RGBA &rgba);
	void setMaterial(uint8_t i, const Material &material);
	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void toVec4f(core::Buffer<glm::vec4> &rgba) const;
	void toVec4f(glm::highp_vec4 *vec4f) const;

	color::RGBA emitColor(uint8_t paletteColorIdx) const;
	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void emitToVec4f(core::Buffer<glm::vec4> &vec4f) const;
	void emitToVec4f(const core::Buffer<glm::vec4> &materialColors, core::Buffer<glm::vec4> &vec4f) const;
	void emitToVec4f(const glm::highp_vec4 *materialColors, glm::highp_vec4 *vec4f) const;
	bool hasAlpha(uint8_t paletteColorIdx) const;
	bool hasEmit(uint8_t paletteColorIdx) const;
	void setMaterialType(uint8_t paletteColorIdx, MaterialType type);
	bool setMaterialProperty(uint8_t paletteColorIdx, const core::String &name, float value);
	float materialProperty(uint8_t paletteColorIdx, const core::String &name) const;

	void setMaterialValue(uint8_t paletteColorIdx, MaterialProperty property, float factor = 1.0f);
	void setEmit(uint8_t paletteColorIdx, float factor = 1.0f);
	void setMetal(uint8_t paletteColorIdx, float factor = 1.0f);
	void setRoughness(uint8_t paletteColorIdx, float factor = 1.0f);
	void setSpecular(uint8_t paletteColorIdx, float factor = 1.0f);
	void setIndexOfRefraction(uint8_t paletteColorIdx, float factor = 1.0f);
	void setAttenuation(uint8_t paletteColorIdx, float factor = 1.0f);
	void setFlux(uint8_t paletteColorIdx, float factor = 1.0f);
	void setAlpha(uint8_t paletteColorIdx, float factor = 1.0f);
	void setDensity(uint8_t paletteColorIdx, float factor = 1.0f);
	void setSp(uint8_t paletteColorIdx, float factor = 1.0f);
	void setPhase(uint8_t paletteColorIdx, float factor = 1.0f);
	void setMedia(uint8_t paletteColorIdx, float factor = 1.0f);
	void setLowDynamicRange(uint8_t paletteColorIdx, float factor = 1.0f);
	// returns true if the palette has any materials set
	bool hasMaterials() const;

	bool hasFreeSlot() const;
	/**
	 * @return The slot for the new color entry or -1 if not possible
	 */
	int duplicateColor(uint8_t paletteColorIdx);
	/**
	 * @brief Tries to remove the given color from the palette. This does not change the indices itself
	 * because that would break existing models using this palette. It tries to set the color slot to empty
	 * and changes the ui ordering accordingly.
	 * @note Always keeps at least one color in the palette
	 */
	bool removeColor(uint8_t paletteColorIdx);
	void changeIntensity(float scale);
	void reduce(uint8_t targetColors);

	const core::String &colorName(uint8_t paletteColorIdx) const;
	void setColorName(uint8_t paletteColorIdx, const core::String &name);

	const core::String &name() const;
	const core::String &filename() const;
	void setName(const core::String &name);
	void setFilename(const core::String &filename);

	uint64_t hash() const;
	int colorCount() const;
	size_t size() const;
	void setSize(int cnt);
	/**
	 * @brief Just increase the size of the palette colors by the given delta
	 */
	int changeSize(int delta);

	bool load(const char *name);
	bool load(const image::ImagePtr &img);
	bool save(const char *name = nullptr) const;

	static core::String print(const Palette &palette, bool colorAsHex = false);

	/**
	 * @brief fill the remaining colors with black
	 */
	void fill();

	static constexpr const char *builtIn[] = {"built-in:nippon", "built-in:minecraft",		   "built-in:magicavoxel",
											  "built-in:quake1", "built-in:commandandconquer", "built-in:starmade"};

	bool minecraft();
	bool magicaVoxel();
	bool nippon();
	bool quake1();
	bool starMade();
	bool commandAndConquer();

	bool isBuiltIn() const;
	static bool isBuiltIn(const core::String &name);
	static bool isLospec(const core::String &name);

	void markDirty() override;
	void markSave();
	bool needsSave() const;
	void markSaved();

	/**
	 * @param rgba Normalized color value [0.0-1.0]
	 * @param skipPaletteColorIdx One particular palette color index that is not taken into account. This can be used to
	 * e.g. search for replacements
	 * @return int The index to the palette color or @c PaletteColorNotFound if no match was found
	 */
	int getClosestMatch(color::RGBA rgba, int skipPaletteColorIdx = -1, color::Distance distance = color::Distance::Approximation) const;
	uint8_t findReplacement(uint8_t paletteColorIdx, color::Distance distance = color::Distance::Approximation) const;
	/**
	 * @brief Will add the given color to the palette - and if the max colors are reached it will try
	 * to match the color to another already existing color in the palette.
	 * @note Only use this for single colors - not for a lot of them. This method is quite slow
	 * @param[in] skipPaletteColorIdx This slot is not filled with any color value - if it's @c -1 every slot is filled
	 */
	bool tryAdd(color::RGBA rgba, bool skipSimilar = true, uint8_t *index = nullptr, bool replaceSimilar = true,
				int skipPaletteColorIdx = -1);
	bool hasColor(color::RGBA rgba);
	void quantize(const color::RGBA *inputColors, const size_t inputColorCount);
	void constrastStretching();
	void whiteBalance();

	static const char *getDefaultPaletteName();
	static core::String extractPaletteName(const core::String &file);
	static bool createPalette(const image::ImagePtr &image, palette::Palette &palette, int imageWidth = -1, int imageHeight = -1);
};

inline PaletteView &Palette::view() {
	return _view;
}

inline const PaletteView &Palette::view() const {
	return _view;
}

inline const core::String &Palette::name() const {
	return _name;
}

inline int Palette::colorCount() const {
	return _colorCount;
}

inline size_t Palette::size() const {
	return _colorCount;
}

inline void Palette::markSave() {
	_needsSave = true;
}

inline bool Palette::needsSave() const {
	return _needsSave;
}

inline void Palette::markSaved() {
	_needsSave = false;
}

inline color::RGBA Palette::color(uint8_t paletteColorIdx) const {
	return _colors[paletteColorIdx];
}

inline color::RGBA Palette::emitColor(uint8_t paletteColorIdx) const {
	if (hasEmit(paletteColorIdx)) {
		return _colors[paletteColorIdx];
	}
	return color::RGBA{};
}

inline const Material &Palette::material(uint8_t paletteColorIdx) const {
	return _materials[paletteColorIdx];
}

} // namespace palette
