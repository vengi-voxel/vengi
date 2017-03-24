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

/**
 * @brief Helper class to pick a random colored @c VoxelType
 */
class RandomVoxel {
private:
	const MaterialColorIndices& indices;
	const core::Random& _random;
	const VoxelType _type;
	int _sameCount;
	mutable int _amount = 1;
	mutable uint8_t _currentIndex = 0u;
public:
	/**
	 * @param type The VoxelType to pick the color index for
	 * @param random @c core::Random instance
	 * @param sameCount The amount of Voxel instances that are returned as the same
	 * color before a possible change in the color index is evaluated.
	 */
	RandomVoxel(VoxelType type, const core::Random& random, int sameCount = 3) :
			indices(getMaterialIndices(type)), _random(random), _type(type), _sameCount(sameCount) {
	}

	inline operator Voxel() const {
		if (indices.size() == 1) {
			return Voxel(_type, indices.front());
		}
		if (_amount == 1) {
			auto i = _random.randomElement(indices.begin(), indices.end());
			_currentIndex = *i;
		}
		++_amount;
		if (_amount >= _sameCount) {
			_amount = 1;
		}
		return Voxel(_type, _currentIndex);
	}
};

}
