/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Color.h"
#include "io/File.h"
#include <vector>
#include "core/Random.h"

namespace voxel {

// this size must match the color uniform size in the shader
typedef std::vector<glm::vec4> MaterialColorArray;
typedef std::vector<uint8_t> MaterialColorIndices;

extern bool initDefaultMaterialColors();
extern bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile);
extern const MaterialColorArray& getMaterialColors();
/**
 * @brief Get all known material color indices for the given VoxelType
 * @param type The VoxelType to get the indices for
 * @return Indices to the MaterialColorArray for the given VoxelType
 */
extern const MaterialColorIndices& getMaterialIndices(VoxelType type);
extern Voxel createRandomColorVoxel(VoxelType type);
extern Voxel createRandomColorVoxel(VoxelType type, core::Random& random);
/**
 * @brief Creates a voxel of the given type with the fixed colorindex that is relative to the
 * valid color indices for this type.
 */
extern Voxel createColorVoxel(VoxelType type, uint32_t colorIndex);

struct RandomVoxel {
	const MaterialColorIndices& indices;
	core::Random random;
	const VoxelType type;

	RandomVoxel(VoxelType _type) :
			indices(getMaterialIndices(_type)), type(_type) {
	}

	RandomVoxel(VoxelType _type, core::Random& _random) :
			indices(getMaterialIndices(_type)), random(_random), type(_type) {
	}

	inline operator Voxel() const {
		if (indices.size() == 1) {
			return Voxel(type, indices.front());
		}
		auto i = random.randomElement(indices.begin(), indices.end());
		return Voxel(type, *i);
	}
};

}
