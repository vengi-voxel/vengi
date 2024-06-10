/**
 * @file
 */

#include "RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

/**
 * @brief A class that records modifications to a RawVolume - but doesn't actually modify it.
 */
class ModificationRecorder {
private:
	const RawVolume &_volume;
	SparseVolume _modifications;

public:
	ModificationRecorder(const RawVolume &volume) : _volume(volume) {
		_modifications.setStoreEmptyVoxels(true);
	}

public:
	class Sampler : public RawVolume::Sampler {
	private:
		using Super = RawVolume::Sampler;

	public:
		Sampler(const ModificationRecorder *volume) : Super(volume->_volume) {
		}

		Sampler(const ModificationRecorder &volume) : Super(volume._volume) {
		}
	};

	inline const Region &region() const {
		return _volume.region();
	}

	inline const Voxel &voxel(const glm::ivec3 &pos) const {
		if (_modifications.hasVoxel(pos)) {
			return _modifications.voxel(pos);
		}
		return _volume.voxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel &voxel(int x, int y, int z) const {
		if (_modifications.hasVoxel(x, y, z)) {
			return _modifications.voxel(x, y, z);
		}
		return _volume.voxel(x, y, z);
	}

	inline bool setVoxel(const glm::ivec3 &pos, const Voxel &voxel) {
		return _modifications.setVoxel(pos, voxel);
	}

	voxel::Region dirtyRegion() const {
		return _modifications.calculateRegion();
	}
};

} // namespace voxel
