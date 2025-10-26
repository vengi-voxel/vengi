/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/DynamicMap.h"
#include "math/Axis.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxel/VolumeSamplerUtil.h"

namespace voxel {

/**
 * Sparse volume implementation which stores data in a hashmap. This is useful for volumes where most of the voxels are
 * empty.
 */
class SparseVolume {
private:
	core::DynamicMap<glm::ivec3, voxel::Voxel, 1031, glm::hash<glm::ivec3>> _map;
	static const constexpr voxel::Voxel _emptyVoxel{VoxelType::Air, 0, 0, 0};
	const voxel::Region _region;
	const bool _isRegionValid;
	bool _storeEmptyVoxels = false;

public:
	class Sampler {
	private:
		static const uint8_t SAMPLER_INVALIDX = 1 << 0;
		static const uint8_t SAMPLER_INVALIDY = 1 << 1;
		static const uint8_t SAMPLER_INVALIDZ = 1 << 2;

	public:
		Sampler(const SparseVolume &volume);
		Sampler(const SparseVolume *volume);
		virtual ~Sampler();

		const Voxel &voxel() const;
		const Region &region() const;

		bool currentPositionValid() const;

		bool setPosition(const glm::ivec3 &pos);
		bool setPosition(int32_t x, int32_t y, int32_t z);
		virtual bool setVoxel(const Voxel &voxel);
		const glm::ivec3 &position() const;

		void movePositiveX(uint32_t offset = 1);
		void movePositiveY(uint32_t offset = 1);
		void movePositiveZ(uint32_t offset = 1);
		void movePositive(math::Axis axis, uint32_t offset = 1);

		void moveNegativeX(uint32_t offset = 1);
		void moveNegativeY(uint32_t offset = 1);
		void moveNegativeZ(uint32_t offset = 1);
		void moveNegative(math::Axis axis, uint32_t offset = 1);

		const Voxel &peekVoxel1nx1ny1nz() const;
		const Voxel &peekVoxel1nx1ny0pz() const;
		const Voxel &peekVoxel1nx1ny1pz() const;
		const Voxel &peekVoxel1nx0py1nz() const;
		const Voxel &peekVoxel1nx0py0pz() const;
		const Voxel &peekVoxel1nx0py1pz() const;
		const Voxel &peekVoxel1nx1py1nz() const;
		const Voxel &peekVoxel1nx1py0pz() const;
		const Voxel &peekVoxel1nx1py1pz() const;

		const Voxel &peekVoxel0px1ny1nz() const;
		const Voxel &peekVoxel0px1ny0pz() const;
		const Voxel &peekVoxel0px1ny1pz() const;
		const Voxel &peekVoxel0px0py1nz() const;
		const Voxel &peekVoxel0px0py0pz() const;
		const Voxel &peekVoxel0px0py1pz() const;
		const Voxel &peekVoxel0px1py1nz() const;
		const Voxel &peekVoxel0px1py0pz() const;
		const Voxel &peekVoxel0px1py1pz() const;

		const Voxel &peekVoxel1px1ny1nz() const;
		const Voxel &peekVoxel1px1ny0pz() const;
		const Voxel &peekVoxel1px1ny1pz() const;
		const Voxel &peekVoxel1px0py1nz() const;
		const Voxel &peekVoxel1px0py0pz() const;
		const Voxel &peekVoxel1px0py1pz() const;
		const Voxel &peekVoxel1px1py1nz() const;
		const Voxel &peekVoxel1px1py0pz() const;
		const Voxel &peekVoxel1px1py1pz() const;

	protected:
		SparseVolume *_volume;

		// The current position in the volume
		glm::ivec3 _posInVolume{0, 0, 0};

		/** Other current position information */
		Voxel _currentVoxel;

		/** Whether the current position is inside the volume */
		uint8_t _currentPositionInvalid = 0u;
	};

	// invalid region means unlimited size
	SparseVolume(const voxel::Region &limitRegion = voxel::Region::InvalidRegion);

	void setStoreEmptyVoxels(bool storeEmptyVoxels) {
		_storeEmptyVoxels = storeEmptyVoxels;
	}

	[[nodiscard]] inline const voxel::Region &region() const {
		return _region;
	}

	[[nodiscard]] Region calculateRegion() const;

	inline bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
		return setVoxel({x, y, z}, voxel);
	}

	bool setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel);

	void clear();

	/**
	 * Gets a voxel at the position given by @c x,y,z coordinates
	 */
	[[nodiscard]] const Voxel &voxel(int32_t x, int32_t y, int32_t z) const {
		return voxel({x, y, z});
	}

	/**
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	[[nodiscard]] const Voxel &voxel(const glm::ivec3 &pos) const;

	[[nodiscard]] bool hasVoxel(int x, int y, int z) const {
		return hasVoxel({x, y, z});
	}

	[[nodiscard]] bool hasVoxel(const glm::ivec3 &pos) const;

	[[nodiscard]] inline bool empty() const {
		return size() == 0;
	}

	[[nodiscard]] inline size_t size() const {
		return _map.size();
	}

	/**
	 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the width is 64.
	 * @sa height(), getDepth()
	 */
	int32_t width() const;

	/**
	 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the height is 64.
	 * @sa width(), getDepth()
	 */
	int32_t height() const;

	/**
	 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the depth is 64.
	 * @sa width(), height()
	 */
	int32_t depth() const;

	template<class Volume>
	void copyTo(Volume &target) const {
		for (auto iter = _map.begin(); iter != _map.end(); ++iter) {
			const glm::ivec3 &pos = iter->first;
			const voxel::Voxel &voxel = iter->second;
			target.setVoxel(pos.x, pos.y, pos.z, voxel);
		}
	}

	template<class Volume>
	void copyFrom(const Volume &source) {
		auto func = [this](int x, int y, int z, const voxel::Voxel &voxel) { setVoxel(x, y, z, voxel); };
		voxelutil::visitVolumeParallel(source, func);
	}
};

template<>
inline bool setVoxels<SparseVolume>(SparseVolume &volume, int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount) {
	const voxel::Region &region = volume.region();
	if (region.isValid()) {
		const int xDiff = region.getLowerX() - x;
		if (xDiff > 0) {
			x += xDiff;
			nx -= xDiff;
		}
		const int yDiff = region.getLowerY() - y;
		if (yDiff > 0) {
			y += yDiff;
			amount -= yDiff;
		}
		const int zDiff = region.getLowerZ() - z;
		if (zDiff > 0) {
			z += zDiff;
			nz -= zDiff;
		}
	}
	for (int lz = 0; lz < nz; ++lz) {
		for (int lx = 0; lx < nx; ++lx) {
			for (int ly = 0; ly < amount; ++ly) {
				volume.setVoxel(x + lx, y + ly, z + lz, voxels[ly]);
			}
		}
	}
	return true;
}

inline int32_t SparseVolume::width() const {
	return _region.getWidthInVoxels();
}

inline int32_t SparseVolume::height() const {
	return _region.getHeightInVoxels();
}

inline int32_t SparseVolume::depth() const {
	return _region.getDepthInVoxels();
}

inline const Region& SparseVolume::Sampler::region() const {
	return _volume->region();
}

inline const glm::ivec3 &SparseVolume::Sampler::position() const {
	return _posInVolume;
}

inline const Voxel &SparseVolume::Sampler::voxel() const {
	if (this->currentPositionValid()) {
		return _currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline bool SparseVolume::Sampler::currentPositionValid() const {
	return !_currentPositionInvalid;
}

inline bool SparseVolume::Sampler::setPosition(const glm::ivec3 &v3dNewPos) {
	return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1ny1nz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1ny0pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1ny1pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx0py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx0py0pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx0py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1py0pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1nx1py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1ny1nz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1ny0pz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1ny1pz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px0py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px0py0pz() const {
	if (this->currentPositionValid()) {
		return _currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px0py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1py0pz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel0px1py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1ny1nz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1ny0pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1ny1pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px0py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px0py0pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px0py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1py1nz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1py0pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &SparseVolume::Sampler::peekVoxel1px1py1pz() const {
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

} // namespace voxel
