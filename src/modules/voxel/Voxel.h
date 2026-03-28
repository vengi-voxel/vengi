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

#define NO_NORMAL 0
#define NORMAL_PALETTE_OFFSET 1

static constexpr const uint8_t FlagOutline = 1;

static constexpr const char* VoxelTypeStr[] = {
	"Air",
	"Transparent",
	"Generic"
};
static_assert(lengthof(VoxelTypeStr) == (int)VoxelType::Max, "voxel type string array size doesn't match the available voxel types");

#ifdef VENGI_COMPACT_VOXEL

/**
 * Compact 1-byte voxel representation for reduced memory usage (4x savings).
 * Bit 7: FlagOutline (selection)
 * Bit 6: solid flag (1 = non-air, 0 = air)
 * Bits 0-5: color index (0-63)
 *
 * Normals, bone indices, and VoxelType::Transparent are not supported.
 * Enable via cmake -DUSE_COMPACT_VOXEL=ON
 */
class Voxel {
public:
	static constexpr uint8_t FLAG_OUTLINE_MASK = 0x80;
	static constexpr uint8_t SOLID_MASK = 0x40;
	static constexpr uint8_t COLOR_MASK = 0x3F;

	CORE_FORCE_INLINE constexpr Voxel() : _data(0) {
	}

	CORE_FORCE_INLINE constexpr Voxel(VoxelType material, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u, uint8_t boneIdx = 0u)
		: _data(material == VoxelType::Air ? 0 : (SOLID_MASK | (colorIndex & COLOR_MASK))) {
		if (flags & FlagOutline) {
			_data |= FLAG_OUTLINE_MASK;
		}
		(void)normalIndex;
		(void)boneIdx;
	}

	CORE_FORCE_INLINE constexpr bool operator==(const Voxel& other) const {
		return _data == other._data;
	}

	CORE_FORCE_INLINE bool isSame(const Voxel& other) const {
		return (_data & (SOLID_MASK | COLOR_MASK)) == (other._data & (SOLID_MASK | COLOR_MASK));
	}

	CORE_FORCE_INLINE Voxel(const Voxel& other) = default;

	CORE_FORCE_INLINE constexpr Voxel &operator=(const Voxel &other) = default;

	CORE_FORCE_INLINE bool isSameType(const Voxel& other) const {
		return getMaterial() == other.getMaterial();
	}

	CORE_FORCE_INLINE uint8_t getColor() const {
		return _data & COLOR_MASK;
	}

	CORE_FORCE_INLINE void setNormal(uint8_t normalIndex) {
		(void)normalIndex;
	}

	CORE_FORCE_INLINE uint8_t getNormal() const {
		return NO_NORMAL;
	}

	CORE_FORCE_INLINE void setColor(uint8_t colorIndex) {
		_data = (_data & (FLAG_OUTLINE_MASK | SOLID_MASK)) | (colorIndex & COLOR_MASK);
	}

	CORE_FORCE_INLINE VoxelType getMaterial() const {
		return (_data & SOLID_MASK) ? VoxelType::Generic : VoxelType::Air;
	}

	CORE_FORCE_INLINE void setMaterial(VoxelType material) {
		if (material == VoxelType::Air) {
			_data = 0;
		} else {
			_data |= SOLID_MASK;
		}
	}

	CORE_FORCE_INLINE uint8_t getFlags() const {
		return (_data & FLAG_OUTLINE_MASK) ? FlagOutline : 0;
	}

	CORE_FORCE_INLINE uint8_t getBoneIdx() const {
		return 0;
	}

	CORE_FORCE_INLINE void setBoneIdx(uint8_t boneIdx) {
		(void)boneIdx;
	}

	void setFlags(uint8_t flags);

	void setOutline() {
		setFlags(FlagOutline);
	}

	void setNormalReset() {
	}

	constexpr bool resetNormal() const {
		return false;
	}

private:
	uint8_t _data;
};
static_assert(sizeof(Voxel) == 1, "Compact voxel size is expected to be 1 byte");

#else // !VENGI_COMPACT_VOXEL

class Voxel {
public:
	CORE_FORCE_INLINE constexpr Voxel()
		: _material(VoxelType::Air), _flags(0), _unused(0), _colorIndex(0), _normalIndex(NO_NORMAL), _boneIdx(0) {
	}

	CORE_FORCE_INLINE constexpr Voxel(VoxelType material, uint8_t colorIndex, uint8_t normalIndex = NO_NORMAL, uint8_t flags = 0u, uint8_t boneIdx = 0u)
		: _material(material), _flags(flags), _unused(0), _colorIndex(colorIndex), _normalIndex(normalIndex), _boneIdx(boneIdx) {
	}

	CORE_FORCE_INLINE constexpr bool operator==(const Voxel& other) const {
		return _material == other._material && _colorIndex == other._colorIndex &&
			   _normalIndex == other._normalIndex && _flags == other._flags && _boneIdx == other._boneIdx;
	}

	CORE_FORCE_INLINE bool isSame(const Voxel& other) const {
		return _material == other._material && _colorIndex == other._colorIndex &&
			   _normalIndex == other._normalIndex;
	}

	CORE_FORCE_INLINE Voxel(const Voxel& other) {
		_material = other._material;
		_flags = other._flags;
		_unused = other._unused;
		_colorIndex = other._colorIndex;
		_normalIndex = other._normalIndex;
		_boneIdx = other._boneIdx;
	}

	CORE_FORCE_INLINE constexpr Voxel &operator=(const Voxel &other) {
		if (other._normalIndex != NO_NORMAL) {
			_normalIndex = other._normalIndex;
		} else if (other.resetNormal()) {
			_normalIndex = NO_NORMAL;
		}
		_material = other._material;
		_flags = other._flags;
		_unused = other._unused;
		_colorIndex = other._colorIndex;
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

	/**
	 * @param[in] normalIndex @c 0 means no normal
	 */
	CORE_FORCE_INLINE void setNormal(uint8_t normalIndex) {
		_normalIndex = normalIndex;
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
		setFlags(FlagOutline);
	}

	void setNormalReset() {
		setFlags(2); // no shader flag for this
	}

	constexpr bool resetNormal() const {
		return (_flags & 2) != 0;
	}

private:
	VoxelType _material:2;
	uint8_t _flags:2; // not all flags are handled in the shader
	uint8_t _unused:4; // VoxelVertex::padding
	uint8_t _colorIndex;
	uint8_t _normalIndex; // 0 == no normal
	uint8_t _boneIdx;
};
static_assert(sizeof(Voxel) == 4, "Voxel size is expected to be 4 bytes");

#endif // VENGI_COMPACT_VOXEL

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
