/**
 * @file
 */

#pragma once

#include "core/DirtyState.h"
#include "color/RGBA.h"
#include "core/collection/Buffer.h"
#include "image/Image.h"
#include <glm/fwd.hpp>

namespace palette {

static const int NormalPaletteMaxNormals = 256;
static const int PaletteNormalNotFound = -1;

/**
 * @brief Some voxel formats are also storing normals in a palette. This is e.g.
 * used for the Command & Conquer voxel formats.
 */
class NormalPalette : public core::DirtyState {
private:
	bool _needsSave = false;
	mutable bool _hashDirty = false;
	core::String _name;
	mutable uint32_t _hash = 0u;
	int _size = 0u;
	color::RGBA _normals[NormalPaletteMaxNormals]{};

public:
	static color::RGBA toRGBA(const glm::vec3 &normal);
	static glm::vec3 toVec3(const color::RGBA &rgba);

	int getClosestMatch(const glm::vec3 &normal) const;
	void loadNormalMap(const color::RGBA *normals, int size);
	void loadNormalMap(const glm::vec3 *normals, int size);

	const color::RGBA &normal(uint8_t index) const;
	glm::vec3 normal3f(uint8_t index) const;
	void setNormal(uint8_t index, const color::RGBA &normal);
	void setNormal(uint8_t index, const glm::vec3 &normal);

	void toVec4f(core::Buffer<glm::vec4> &normals) const;
	void toVec4f(glm::highp_vec4 *vec4f) const;

	const core::String &name() const;
	void setName(const core::String &name);
	uint32_t hash() const;
	size_t size() const;
	bool load(const char *name);
	bool load(const image::ImagePtr &img);
	bool save(const char *name = nullptr) const;

	static constexpr const char *builtIn[] = {"built-in:redalert2", "built-in:tiberiansun", "built-in:slab6"};
	bool isBuiltIn() const;
	bool isTiberianSun() const;
	bool isRedAlert2() const;
	void tiberianSun();
	void redAlert2();
	void slab6();

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

inline void NormalPalette::setNormal(uint8_t index, const color::RGBA &normal) {
	_normals[index] = normal;
}

inline const color::RGBA &NormalPalette::normal(uint8_t index) const {
	return _normals[index];
}

} // namespace palette
