/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "core/DirtyState.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "core/RGBA.h"
#include <cstdint>
#include <stdint.h>
#include <glm/vec4.hpp>

namespace palette {

static const int PaletteMaxColors = 256;
static const int PaletteColorNotFound = -1;

// unused in vengi at the moment, but here to provide a better magicavoxel import/export experience
enum class MaterialType {
	Diffuse = 0, // diffuse is default
	Metal = 1,
	Glass = 2,
	Emit = 3,
	Blend = 4,
	Media = 5,
};

// just a few of these values are used for rendering in vengi - but all are used for import/export if the format
// supports it
enum MaterialProperty : uint32_t {
	MaterialNone = 0,

	MaterialMetal = 1,
	MaterialRoughness = 2,
	MaterialSpecular = 3,
	MaterialIndexOfRefraction = 4,
	MaterialAttenuation = 5,
	MaterialFlux = 6,
	MaterialEmit = 7,
	MaterialLowDynamicRange = 8,
	MaterialDensity = 9,
	MaterialSp = 10,
	MaterialGlossiness = 11,
	MaterialMedia = 12,

	MaterialMax
};

struct Material {
	uint32_t mask = MaterialNone;
	MaterialType type = MaterialType::Diffuse;
	// make sure to keep the order of the properties and keep metal
	// as first - see the name string array
	float metal = 0.0f;
	float roughness = 0.1f;
	float specular = 0.0f;
	float indexOfRefraction = 1.3f;
	float attenuation = 0.0f;
	float flux = 0.0f;
	float emit = 0.0f;
	float lowDynamicRange = 0.0f;
	float density = 0.0f;
	float sp = 0.0f;
	float glossiness = 0.0f;
	float media = 0.0f;

	bool operator==(const Material &rhs) const;
	bool operator!=(const Material &rhs) const;
	bool has(MaterialProperty n) const;
	float value(MaterialProperty n) const;
	void setValue(MaterialProperty n, float value);
};

// make sure to keep the order of the properties - see Material struct float values
static constexpr const char *MaterialPropertyNames[] = {
	"metal",
	"roughness",
	"specular",
	"indexOfRefraction",
	"attenuation",
	"flux",
	"emit",
	"lowDynamicRange",
	"density",
	"sp",
	"glossiness",
	"media"
};
static_assert(lengthof(MaterialPropertyNames) == MaterialMax - 1, "MaterialPropertyNames size mismatch");

inline const char *MaterialPropertyName(MaterialProperty prop) {
	core_assert(prop > MaterialNone && prop < MaterialMax);
	return MaterialPropertyNames[(int)prop - 1];
}

// make sure to keep the order of the properties - see Material struct float values
struct MaterialMinMax {
	float minVal;
	float maxVal;
};
static constexpr const MaterialMinMax MaterialPropertyMinsMaxs[] = {
	{0.0f, 1.0f}, // metal
	{0.0f, 1.0f}, // roughness
	{0.0f, 1.0f}, // specular
	{0.0f, 3.0f}, // indexOfRefraction
	{0.0f, 1.0f}, // attenuation
	{0.0f, 1.0f}, // flux
	{0.0f, 1.0f}, // emit
	{0.0f, 1.0f}, // lowDynamicRange
	{0.0f, 1.0f}, // density
	{0.0f, 1.0f}, // sp
	{0.0f, 1.0f}, // glossiness
	{0.0f, 1.0f}  // media
};
static_assert(lengthof(MaterialPropertyNames) == MaterialMax - 1, "MaterialPropertyNames size mismatch");
inline MaterialMinMax MaterialPropertyMinMax(MaterialProperty prop) {
	core_assert(prop > MaterialNone && prop < MaterialMax);
	return MaterialPropertyMinsMaxs[(int)prop - 1];
}

// RGBA color values in the range [0-255]
using PaletteColorArray = core::RGBA[PaletteMaxColors];
using PaletteIndicesArray = uint8_t[PaletteMaxColors];
using MaterialArray = Material[PaletteMaxColors];

class Palette : public core::DirtyState {
private:
	bool _needsSave = false;
	core::String _name;
	union hash {
		uint32_t _hashColors[2];
		uint64_t _hash;
	} _hash {};

	bool load(const uint8_t *rgbaBuf, size_t bufsize, const char *name);
	PaletteColorArray _colors {};
	MaterialArray _materials {};
	int _colorCount = 0;
	PaletteIndicesArray _indices;

	bool loadLospec(const core::String &lospecId, const core::String &gimpPalette);
public:
	Palette();

	/**
	 * In case the palette indices are changed, this gives you access to the real texture index
	 */
	uint8_t index(uint8_t idx) const;
	const PaletteIndicesArray &indices() const;
	PaletteIndicesArray &indices();
	/**
	 * @note Only for ui purposes - changes the color slots
	 */
	void exchange(uint8_t idx1, uint8_t idx2);

	const Material &material(uint8_t idx) const;
	core::RGBA color(uint8_t idx) const;
	glm::vec4 color4(uint8_t idx) const;
	void setColor(uint8_t idx, const core::RGBA &rgba);
	void setMaterial(uint8_t i, const Material &material);
	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void toVec4f(core::DynamicArray<glm::vec4> &rgba) const;

	core::RGBA emitColor(uint8_t idx) const;
	/**
	 * @brief Convert the RGBA color values in the range [0-255] to float color values in the range [0.0-1.0]
	 * @note The collection will have 256 entries - even if the palette has less entries
	 */
	void emitToVec4f(core::DynamicArray<glm::vec4> &vec4f) const;

	bool hasAlpha(uint8_t idx) const;
	bool hasEmit(uint8_t idx) const;
	void setMaterialType(uint8_t idx, MaterialType type);
	bool setMaterialProperty(uint8_t idx, const core::String &name, float value);
	float materialProperty(uint8_t idx, const core::String &name) const;

	void setMaterialValue(uint8_t idx, MaterialProperty property, float factor = 1.0f);
	void setEmit(uint8_t idx, float factor = 1.0f);
	void setMetal(uint8_t idx, float factor = 1.0f);
	void setRoughness(uint8_t idx, float factor = 1.0f);
	void setSpecular(uint8_t idx, float factor = 1.0f);
	void setIndexOfRefraction(uint8_t idx, float factor = 1.0f);
	void setAttenuation(uint8_t idx, float factor = 1.0f);
	void setFlux(uint8_t idx, float factor = 1.0f);
	void setAlpha(uint8_t idx, float factor = 1.0f);
	void setDensity(uint8_t idx, float factor = 1.0f);
	void setSp(uint8_t idx, float factor = 1.0f);
	void setGlossiness(uint8_t idx, float factor = 1.0f);
	void setMedia(uint8_t idx, float factor = 1.0f);
	void setLowDynamicRange(uint8_t idx, float factor = 1.0f);

	bool hasFreeSlot() const;
	void duplicateColor(uint8_t idx);
	/**
	 * @brief Tries to remove the given color from the palette
	 * @note Always keeps at least one color in the palette
	 */
	bool removeColor(uint8_t idx);
	void changeIntensity(float scale);
	void reduce(uint8_t targetColors);

	const core::String &name() const;
	void setName(const core::String &name);
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

	void sortOriginal();
	void sortHue();
	void sortSaturation();
	void sortBrightness();
	void sortCIELab();

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

	bool isBuiltIn() const;

	void markDirty() override;
	void markSave();
	bool needsSave() const;
	void markSaved();

	/**
	 * @param rgba Normalized color value [0.0-1.0]
	 * @param distance Optional parameter to get the calculated distance for the selected color entry
	 * @param skip One particular palette color index that is not taken into account. This can be used to e.g. search for replacements
	 * @return int The index to the palette color or @c PaletteColorNotFound if no match was found
	 */
	int getClosestMatchWithDistance(const core::RGBA rgba, int skip = -1, float *distance = nullptr) const;
	int getClosestMatch(const core::RGBA rgba, int skip = -1) const {
		return getClosestMatchWithDistance(rgba, skip);
	}
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

	static const char* getDefaultPaletteName();
	static core::String extractPaletteName(const core::String& file);
	static bool createPalette(const image::ImagePtr& image, palette::Palette &palette);
	static bool convertImageToPalettePng(const image::ImagePtr& image, const char *paletteFile);
};

inline const core::String &Palette::name() const {
	return _name;
}

inline void Palette::setName(const core::String &name) {
	_name = name;
}

inline uint64_t Palette::hash() const {
	return _hash._hash;
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

inline const PaletteIndicesArray &Palette::indices() const {
	return _indices;
}

inline PaletteIndicesArray &Palette::indices(){
	return _indices;
}

inline uint8_t Palette::index(uint8_t idx) const {
	return _indices[idx];
}

inline core::RGBA Palette::color(uint8_t idx) const {
	return _colors[idx];
}

inline core::RGBA Palette::emitColor(uint8_t idx) const {
	if (hasEmit(idx)) {
		return _colors[idx];
	}
	return core::RGBA{};
}

inline const Material &Palette::material(uint8_t idx) const {
	return _materials[idx];
}

} // namespace voxel
