/**
 * @file
 * @defgroup Voxel
 * @{
 */

#pragma once

#include <stdint.h>
#include "core/ArrayLength.h"

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

static constexpr const char* VoxelTypeStr[] = {
	"Air",
	"Transparent",
	"Generic"
};
static_assert(lengthof(VoxelTypeStr) == (int)VoxelType::Max, "voxel type string array size doesn't match the available voxel types");

/**
 * @return VoxelType::Max if invalid string was given
 */
extern VoxelType getVoxelType(const char *str);

class Voxel {
public:
	constexpr inline Voxel() :
		_material(VoxelType::Air), _flags(0), _colorIndex(0) {
	}

	constexpr inline Voxel(VoxelType material, uint8_t colorIndex, uint8_t flags = 0u) :
		_material(material), _flags(flags), _colorIndex(colorIndex) {
	}

	constexpr inline Voxel(const Voxel& voxel) :
		_material(voxel._material), _flags(voxel._flags), _colorIndex(voxel._colorIndex) {
	}

	constexpr inline Voxel& operator=(const Voxel& voxel) {
		_material = voxel._material;
		_colorIndex = voxel._colorIndex;
		_flags = voxel._flags;
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
		return _material == other._material && _colorIndex == other._colorIndex;
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

	void setBloom() {
		setFlags(2); // FlagBloom
	}

	void setOutline() {
		setFlags(1); // FlagOutline
	}

private:
	VoxelType _material:5;
	uint8_t _flags:3;
	uint8_t _colorIndex;
};

constexpr Voxel createVoxel(VoxelType type, uint8_t colorIndex) {
	return Voxel(type, colorIndex);
}

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
