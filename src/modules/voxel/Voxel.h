/**
 * @file
 */

#pragma once

#include "polyvox/Voxel.h"
#include "polyvox/Mesh.h"
#include "polyvox/Vertex.h"
#include "polyvox/CubicSurfaceExtractor.h"

namespace voxel {

#define MAX_HEIGHT 255
#define MAX_TERRAIN_HEIGHT 100
#define MAX_MOUNTAIN_HEIGHT (MAX_TERRAIN_HEIGHT + 60)
#define MAX_WATER_HEIGHT 10

constexpr Voxel createVoxel(VoxelType type) {
	return Voxel(type);
}

typedef Mesh<Vertex> DecodedMesh;

struct DecodedMeshData {
	DecodedMesh mesh;
	// translation on the x and z axis
	glm::ivec3 translation;
};

inline bool isBlocked(VoxelType material) {
	return material != Air;
}


inline bool isLeaves(VoxelType material) {
	return material >= Leaves1 && material <= Leaves10;
}

inline bool isAir(VoxelType material) {
	return material == Air;
}

inline bool isWood(VoxelType material) {
	return material == Wood1;
}

inline bool isGrass(VoxelType material) {
	return material == Grass1;
}

inline bool isRock(VoxelType material) {
	return material == Rock1 ||material == Rock2 || material == Rock3 || material == Rock4;
}

inline bool isSand(VoxelType material) {
	return material == Sand1 ||material == Sand2 || material == Sand3 || material == Sand4;
}

inline bool isDirt(VoxelType material) {
	return material == Dirt1 ||material == Dirt2 || material == Dirt3 || material == Dirt4;
}

inline bool isFloor(VoxelType material) {
	return isRock(material) || isDirt(material) || isSand(material) || isGrass(material);
}

struct IsVoxelTransparent {
	IsVoxelTransparent() {}
	inline bool operator()(const Voxel& voxel) const {
		return voxel.getMaterial() == Air;
	}
};

/**
 * @brief The criteria used here are that the voxel in front of the potential
 * quad should have a Air voxel while the voxel behind the potential quad
 * would have a voxel that is not Air.
 */
struct IsQuadNeeded {
	inline bool operator()(const Voxel& back, const Voxel& front, Voxel& materialToUse, FaceNames face, int x, int z) const {
		if (back.getMaterial() != Air && front.getMaterial() == Air) {
			materialToUse = back;
			return true;
		}
		if (back.getMaterial() != Water && front.getMaterial() == Water) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

}
