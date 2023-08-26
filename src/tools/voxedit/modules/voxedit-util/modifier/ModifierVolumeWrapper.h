/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "ModifierType.h"
#include "Selection.h"

namespace voxedit {

/**
 * @brief A wrapper for a @c voxel::RawVolume that performs a sanity check for
 * the @c setVoxel() call and uses the @c ModifierType value to perform the
 * desired action for the @c setVoxel() call.
 * The sanity check also includes the @c Selections that are used to limit the
 * area of the @c voxel::RawVolume that is affected by the @c setVoxel() call.
 */
class ModifierVolumeWrapper {
private:
	voxel::RawVolume* _volume;
	const Selections _selections;
	voxel::Region _region;
	voxel::Region _dirtyRegion = voxel::Region::InvalidRegion;
	const ModifierType _modifierType;

	bool _eraseVoxels;
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

	ModifierVolumeWrapper(voxel::RawVolume* volume, ModifierType modifierType, const Selections &selections) :
			_volume(volume), _selections(selections), _region(volume->region()), _modifierType(modifierType) {
		_eraseVoxels = (_modifierType & ModifierType::Erase) == ModifierType::Erase;
		_overwrite = (_modifierType & ModifierType::Place) == ModifierType::Place && _eraseVoxels;
		_update = (_modifierType & ModifierType::Paint) == ModifierType::Paint;
		_force = _overwrite || _eraseVoxels;
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

	const voxel::Region& dirtyRegion() const {
		return _dirtyRegion;
	}

	void addDirtyRegion(const voxel::Region& region) {
		if (!region.isValid()) {
			return;
		}
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(region);
		} else {
			_dirtyRegion = region;
		}
	}

	bool skip(int x, int y, int z) const {
		// prevent out of bounds access with the real volume region
		// the region in this wrapper can exceed the boundaries of
		// the real volume
		if (!_volume->region().containsPoint(x, y, z)) {
			return true;
		}
		if (_selections.empty()) {
			return !_region.containsPoint(x, y, z);
		}
		return !contains(_selections, x, y, z);
	}

	/**
	 * @return @c false if the voxel was not placed because the given position is outside of the valid region, @c
	 * true if the voxel was placed in the region.
	 * @note The return values have a different meaning as for the wrapped voxel::RawVolume.
	 */
	bool setVoxel(int x, int y, int z, const voxel::Voxel& voxel) {
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
		if (skip(x, y, z)) {
			return false;
		}
		voxel::Voxel placeVoxel = voxel;
		if (!_overwrite && _eraseVoxels) {
			placeVoxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
		}
		const glm::ivec3 p(x, y, z);
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
				for (int ny = 0; ny < amount; ++ny) {
					setVoxel(x + j, ny + y, z + k, voxels[ny]);
				}
			}
		}
		return true;
	}

	void setRegion(const voxel::Region &region) {
		_region = region;
	}
};

}
