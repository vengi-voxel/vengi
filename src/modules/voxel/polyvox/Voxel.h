/**
 * @file
 */

#pragma once

#include <cstdint>

namespace voxel {

/**
 * @brief material types 0 - 255 (8 bits)
 * This is basically the index in the color array
 */
enum class VoxelType : uint8_t {
	// this must be 0
	Air = 0,
	Grass1 = 1,
	Wood1 = 2,
	Leaves1 = 3,
	Leaves2 = 4,
	Leaves3 = 5,
	Leaves4 = 6,
	Leaves5 = 7,
	Leaves6 = 8,
	Leaves7 = 9,
	Leaves8 = 10,
	Leaves9 = 11,
	Leaves10 = 12,
	Rock1 = 13,
	Rock2 = 14,
	Rock3 = 15,
	Rock4 = 16,
	Sand1 = 17,
	Sand2 = 18,
	Sand3 = 19,
	Sand4 = 20,
	Cloud = 21,
	Water = 22,
	Dirt1 = 23,
	Dirt2 = 24,
	Dirt3 = 25,
	Dirt4 = 26,
};

class Voxel {
public:
	constexpr Voxel() :
			_material(VoxelType::Air) {
	}

	constexpr Voxel(VoxelType material) :
			_material(material) {
	}

	constexpr Voxel(const Voxel& voxel) :
			_material(voxel._material) {
	}

	inline bool operator==(const Voxel& rhs) const {
		return _material == rhs._material;
	}

	inline bool operator!=(const Voxel& rhs) const {
		return !(*this == rhs);
	}

	inline VoxelType getMaterial() const {
		return _material;
	}

	inline void setMaterial(VoxelType material) {
		_material = material;
	}

private:
	VoxelType _material;
};

constexpr Voxel createVoxel(VoxelType type) {
	return Voxel(type);
}

inline bool isBlocked(VoxelType material) {
	return material != VoxelType::Air;
}

inline bool isWater(VoxelType material) {
	return material == VoxelType::Water;
}

inline bool isLeaves(VoxelType material) {
	return material >= VoxelType::Leaves1 && material <= VoxelType::Leaves10;
}

inline bool isAir(VoxelType material) {
	return material == VoxelType::Air;
}

inline bool isWood(VoxelType material) {
	return material == VoxelType::Wood1;
}

inline bool isGrass(VoxelType material) {
	return material == VoxelType::Grass1;
}

inline bool isRock(VoxelType material) {
	return material == VoxelType::Rock1 ||material == VoxelType::Rock2 || material == VoxelType::Rock3 || material == VoxelType::Rock4;
}

inline bool isSand(VoxelType material) {
	return material == VoxelType::Sand1 ||material == VoxelType::Sand2 || material == VoxelType::Sand3 || material == VoxelType::Sand4;
}

inline bool isDirt(VoxelType material) {
	return material == VoxelType::Dirt1 ||material == VoxelType::Dirt2 || material == VoxelType::Dirt3 || material == VoxelType::Dirt4;
}

inline bool isFloor(VoxelType material) {
	return isRock(material) || isDirt(material) || isSand(material) || isGrass(material);
}

}
