/**
 * @file
 * @defgroup Voxel Voxel
 * @{
 */

#pragma once

#include <stdint.h>
#include "core/ArrayLength.h"

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
	constexpr inline Voxel()
		: _material(VoxelType::Air), _flags(0), _unused(0), _colorIndex(0), _normalIndex(NO_NORMAL), _unused2(0) {
	}

	constexpr inline Voxel(VoxelType material, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u)
		: _material(material), _flags(flags), _unused(0), _colorIndex(colorIndex), _normalIndex(normalIndex), _unused2(0) {
	}

	constexpr inline Voxel(const Voxel &voxel)
		: _material(voxel._material), _flags(voxel._flags), _unused(voxel._unused), _colorIndex(voxel._colorIndex),
		  _normalIndex(voxel._normalIndex), _unused2(voxel._unused2) {
	}

	constexpr inline Voxel& operator=(const Voxel& voxel) {
		_material = voxel._material;
		_colorIndex = voxel._colorIndex;
		_flags = voxel._flags;
		_unused = voxel._unused;
		_normalIndex = voxel._normalIndex;
		_unused2 = voxel._unused2;
		return *this;
	}

	/**
	 * @brief Compares by the material type
	 */
	inline bool operator==(const Voxel& rhs) const {
		return _material == rhs._material;
	}

	/**
	 * @brief Compares by the material type
	 */
	inline bool operator!=(const Voxel& rhs) const {
		return !(*this == rhs);
	}

	inline bool isSame(const Voxel& other) const {
		return _material == other._material && _colorIndex == other._colorIndex && _normalIndex == other._normalIndex;
	}

	/**
	 * @brief Compares by the material type
	 */
	inline bool isSameType(const Voxel& other) const {
		return _material == other._material;
	}

	inline uint8_t getColor() const {
		return _colorIndex;
	}

	inline uint8_t getNormal() const {
		return _normalIndex;
	}

	inline void setColor(uint8_t colorIndex) {
		_colorIndex = colorIndex;
	}

	inline VoxelType getMaterial() const {
		return _material;
	}

	inline void setMaterial(VoxelType material) {
		_material = material;
	}

	inline uint8_t getFlags() const {
		return _flags;
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
public:
	uint8_t _unused2; // used to store the ambient occlusion value for the voxel in the VoxelVertex struct
};
static_assert(sizeof(Voxel) == 4, "Voxel size is expected to be 4 bytes");

constexpr Voxel createVoxel(VoxelType type, uint8_t colorIndex, uint8_t normalIndex = 0u, uint8_t flags = 0u) {
	return Voxel(type, colorIndex, normalIndex, flags);
}

voxel::Voxel createVoxel(const palette::Palette &pal, uint8_t index, uint8_t normalIndex = 0u, uint8_t flags = 0u);

inline bool isBlocked(VoxelType material) {
	return material != VoxelType::Air;
}

inline bool isAir(VoxelType material) {
	return material == VoxelType::Air;
}

inline bool isTransparent(VoxelType material) {
	return material == VoxelType::Transparent;
}

}

/**
 * @}
 */
