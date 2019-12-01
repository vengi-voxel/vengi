/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "ModifierType.h"
#include "Selection.h"

namespace voxedit {

/**
 * @brief A wrapper for a voxel::RawVolume that performs a sanity check for the setVoxel call.
 */
class ModifierVolumeWrapper {
private:
	voxel::RawVolume* _volume;
	const voxel::Region& _region;
	voxel::Region _dirtyRegion = voxel::Region::InvalidRegion;
	const ModifierType _modifierType;

	bool _deleteVoxels;
	bool _overwrite;
	bool _update;
	bool _force;

public:
	class Sampler : public voxel::RawVolume::Sampler {
	private:
		using Super = voxel::RawVolume::Sampler;
	public:
		Sampler(const ModifierVolumeWrapper* volume) : Super(volume->volume()) {}

		Sampler(const ModifierVolumeWrapper& volume) : Super(volume.volume()) {};

		Sampler(ModifierVolumeWrapper* volume) : Super(volume->volume()) {}

		Sampler(ModifierVolumeWrapper& volume) : Super(volume.volume()) {};
	};

	ModifierVolumeWrapper(voxel::RawVolume* volume, ModifierType modifierType) :
			_volume(volume), _region(volume->region()), _modifierType(modifierType) {
		_deleteVoxels = (_modifierType & ModifierType::Delete) == ModifierType::Delete;
		_overwrite = (_modifierType & ModifierType::Place) == ModifierType::Place && _deleteVoxels;
		_update = (_modifierType & ModifierType::Update) == ModifierType::Update;
		_force = _overwrite || _deleteVoxels;
	}

	inline operator voxel::RawVolume& () const {
		return *_volume;
	}

	inline operator const voxel::RawVolume& () const {
		return *_volume;
	}

	inline operator voxel::RawVolume* () const {
		return _volume;
	}

	inline operator const voxel::RawVolume* () const {
		return _volume;
	}

	inline voxel::RawVolume* volume() const {
		return _volume;
	}

	inline const voxel::Region& region() const {
		return _region;
	}

	inline const voxel::Voxel& voxel(const glm::ivec3& pos) const {
		return _volume->voxel(pos.x, pos.y, pos.z);
	}

	inline const voxel::Voxel& voxel(int x, int y, int z) const {
		return _volume->voxel(x, y, z);
	}

	inline bool setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline const voxel::Region& dirtyRegion() const {
		return _dirtyRegion;
	}

	/**
	 * @return @c false if the voxel was not placed because the given position is outside of the valid region, @c
	 * true if the voxel was placed in the region.
	 * @note The return values have a different meaning as for the wrapped voxel::RawVolume.
	 */
	inline bool setVoxel(int x, int y, int z, const voxel::Voxel& voxel) {
		if (!_force) {
			const bool empty = voxel::isAir(this->voxel(x, y, z).getMaterial());
			if (_update) {
				if (empty) {
					return false;
				}
			} else if (!empty) {
				return false;
			}
		}
		const glm::ivec3 p(x, y, z);
		if (!_region.containsPoint(p)) {
			return false;
		}
		voxel::Voxel placeVoxel = voxel;
		if (!_overwrite && _deleteVoxels) {
			placeVoxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
		}
		if (_volume->setVoxel(p, placeVoxel)) {
			if (_dirtyRegion.isValid()) {
				_dirtyRegion.accumulate(p);
			} else {
				_dirtyRegion = voxel::Region(p, p);
			}
		}
		return true;
	}

	inline bool setVoxels(int x, int z, const voxel::Voxel* voxels, int amount) {
		for (int y = 0; y < amount; ++y) {
			setVoxel(x, y, z, voxels[y]);
		}
		return true;
	}

	inline bool setVoxels(int x, int y, int z, int nx, int nz, const voxel::Voxel* voxels, int amount) {
		for (int j = 0; j < nx; ++j) {
			for (int k = 0; k < nz; ++k) {
				for (int ny = y; ny < amount; ++ny) {
					setVoxel(x + j, ny, z + k, voxels[ny]);
				}
			}
		}
		return true;
	}
};

}
