/**
 * @file
 * @defgroup Voxel Voxel
 * @{
 */

#pragma once

#include <stdint.h>
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "color/RGBA.h"

namespace palette {
class Palette;
}

/**
 * Voxel manipulation, meshing and storage
 */
namespace voxel {

/**
 * @brief material types 0 - 31 (5 bits)
 * @note These must match the compute kernel source enum
 */
enum class VoxelType : uint8_t {
	// this must be 0
	Air = 0,
	Transparent,
	Generic,

	Max
};

#define NO_NORMAL 255u

static constexpr const char* VoxelTypeStr[] = {
	"Air",
	"Transparent",
	"Generic"
};
static_assert(lengthof(VoxelTypeStr) == (int)VoxelType::Max, "voxel type string array size doesn't match the available voxel types");

class Voxel {
public:
	CORE_FORCE_INLINE constexpr Voxel()
		: _material(VoxelType::Air), _flags(0), _unused(0), _colorIndex(0), _normalIndex(NO_NORMAL), _boneIdx(0) {
	}

	CORE_FORCE_INLINE constexpr Voxel(VoxelType material, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u, uint8_t boneIdx = 0u)
		: _material(material), _flags(flags), _unused(0), _colorIndex(colorIndex), _normalIndex(normalIndex), _boneIdx(boneIdx) {
	}

	CORE_FORCE_INLINE bool isSame(const Voxel& other) const {
		return _material == other._material && _colorIndex == other._colorIndex && _normalIndex == other._normalIndex;
	}

	CORE_FORCE_INLINE Voxel(const Voxel& other) {
		_material = other._material;
		_flags = other._flags;
		_colorIndex = other._colorIndex;
		_normalIndex = other._normalIndex;
		_boneIdx = other._boneIdx;
	}

	CORE_FORCE_INLINE constexpr Voxel &operator=(const Voxel &other) {
		uint8_t nidx = other._normalIndex;
		if (_normalIndex != NO_NORMAL) {
			nidx = _normalIndex;
		}
		_material = other._material;
		_flags = other._flags;
		_colorIndex = other._colorIndex;
		_normalIndex = nidx;
		_boneIdx = other._boneIdx;
		return *this;
	}

	/**
	 * @brief Compares by the material type
	 */
	CORE_FORCE_INLINE bool isSameType(const Voxel& other) const {
		return _material == other._material;
	}

	CORE_FORCE_INLINE uint8_t getColor() const {
		return _colorIndex;
	}

	CORE_FORCE_INLINE uint8_t getNormal() const {
		return _normalIndex;
	}

	CORE_FORCE_INLINE void setColor(uint8_t colorIndex) {
		_colorIndex = colorIndex;
	}

	CORE_FORCE_INLINE VoxelType getMaterial() const {
		return _material;
	}

	CORE_FORCE_INLINE void setMaterial(VoxelType material) {
		_material = material;
	}

	CORE_FORCE_INLINE uint8_t getFlags() const {
		return _flags;
	}

	CORE_FORCE_INLINE uint8_t getBoneIdx() const {
		return _boneIdx;
	}

	CORE_FORCE_INLINE void setBoneIdx(uint8_t boneIdx) {
		_boneIdx = boneIdx;
	}

	void setFlags(uint8_t flags);

	void setOutline() {
		setFlags(1); // FlagOutline
	}

private:
	VoxelType _material:2;
	uint8_t _flags:1;
	uint8_t _unused:5; // VoxelVertex::padding
	uint8_t _colorIndex;
	uint8_t _normalIndex; // 255 is not set
	uint8_t _boneIdx;
};
static_assert(sizeof(Voxel) == 4, "Voxel size is expected to be 4 bytes");

CORE_NO_SANITIZE_ADDRESS constexpr Voxel createVoxel(VoxelType type, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u, uint8_t boneIdx = 0u) {
	return Voxel(type, colorIndex, normalIndex, flags, boneIdx);
}

voxel::Voxel createVoxelFromColor(const palette::Palette &pal, color::RGBA color);
CORE_NO_SANITIZE_ADDRESS voxel::Voxel createVoxel(const palette::Palette &pal, uint8_t index, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u, uint8_t boneIdx = 0u);

CORE_FORCE_INLINE bool isBlocked(VoxelType material) {
	return material != VoxelType::Air;
}

CORE_FORCE_INLINE bool isAir(VoxelType material) {
	return material == VoxelType::Air;
}

CORE_FORCE_INLINE bool isTransparent(VoxelType material) {
	return material == VoxelType::Transparent;
}

}

/**
 * @}
 */
