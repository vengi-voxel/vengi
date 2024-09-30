/**
 * @file
 */

#pragma once

#include "core/DirtyState.h"
#include "core/RGBA.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include <glm/fwd.hpp>

namespace palette {

static const int NormalPaletteMaxNormals = 255;

class NormalPalette : public core::DirtyState {
private:
	bool _needsSave = false;
	core::String _name;
	uint32_t _hash;
	uint8_t _size = 0;
	core::RGBA _normals[NormalPaletteMaxNormals]{};

public:
	uint8_t getClosestMatch(const glm::vec3 &normal) const;
	void loadNormalMap(const core::RGBA *normals, uint8_t size);
	void loadNormalMap(const glm::vec3 *normals, uint8_t size);

	const core::RGBA &normal(uint8_t index) const;
	glm::vec3 normal3f(uint8_t index) const;
	void setNormal(uint8_t index, const core::RGBA &normal);
	void setNormal(uint8_t index, const glm::vec3 &normal);

	void toVec4f(core::DynamicArray<glm::vec4> &normals) const;

	const core::String &name() const;
	void setName(const core::String &name);
	uint32_t hash() const;
	size_t size() const;
	bool load(const char *name);
	bool load(const image::ImagePtr &img);
	bool save(const char *name = nullptr) const;

	static constexpr const char *builtIn[] = {"built-in:redalert2", "built-in:tiberiansun"};
	bool isBuiltIn() const;
	bool isTiberianSun() const;
	bool isRedAlert2() const;
	void tiberianSun();
	void redAlert2();

	void markDirty() override;
	void markSave();
	bool needsSave() const;
	void markSaved();
};

inline const core::String &NormalPalette::name() const {
	return _name;
}

inline void NormalPalette::setName(const core::String &name) {
	_name = name;
}

inline uint32_t NormalPalette::hash() const {
	return _hash;
}

inline void NormalPalette::markSave() {
	_needsSave = true;
}

inline bool NormalPalette::needsSave() const {
	return _needsSave;
}

inline void NormalPalette::markSaved() {
	_needsSave = false;
}

inline size_t NormalPalette::size() const {
	return _size;
}

inline void NormalPalette::setNormal(uint8_t index, const core::RGBA &normal) {
	_normals[index] = normal;
}

inline const core::RGBA &NormalPalette::normal(uint8_t index) const {
	return _normals[index];
}

} // namespace palette
