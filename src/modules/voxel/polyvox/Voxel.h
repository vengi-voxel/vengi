#pragma once

#include <cstdint>

namespace voxel {

// material types 0 - 255 (8 bits)
typedef uint8_t VoxelType;

static const VoxelType Invalid = -1;
// this must be 0
static const VoxelType Air = 0;
static const VoxelType Grass1 = 1;
static const VoxelType Wood1 = 2;
static const VoxelType Leaves1 = 3;
static const VoxelType Leaves2 = 4;
static const VoxelType Leaves3 = 5;
static const VoxelType Leaves4 = 6;
static const VoxelType Leaves5 = 7;
static const VoxelType Leaves6 = 8;
static const VoxelType Leaves7 = 9;
static const VoxelType Leaves8 = 10;
static const VoxelType Leaves9 = 11;
static const VoxelType Leaves10 = 12;
static const VoxelType Rock1 = 13;
static const VoxelType Rock2 = 14;
static const VoxelType Rock3 = 15;
static const VoxelType Rock4 = 16;
static const VoxelType Sand1 = 17;
static const VoxelType Sand2 = 18;
static const VoxelType Sand3 = 19;
static const VoxelType Sand4 = 20;
static const VoxelType Cloud = 21;
static const VoxelType Water = 22;
static const VoxelType Dirt1 = 23;
static const VoxelType Dirt2 = 24;
static const VoxelType Dirt3 = 25;
static const VoxelType Dirt4 = 26;

class Voxel {
public:
	Voxel() :
			m_uMaterial(Air) {
	}

	Voxel(VoxelType uMaterial) :
			m_uMaterial(uMaterial) {
	}

	bool operator==(const Voxel& rhs) const {
		return m_uMaterial == rhs.m_uMaterial;
	}

	bool operator!=(const Voxel& rhs) const {
		return !(*this == rhs);
	}

	VoxelType getMaterial() const {
		return m_uMaterial;
	}

	void setMaterial(VoxelType uMaterial) {
		m_uMaterial = uMaterial;
	}

private:
	VoxelType m_uMaterial;
};

}
