/**
 * @file
 */

#pragma once

#include "Region.h"
#include "Voxel.h"
#include "core/collection/DynamicArray.h"
#include "VolumeSampler.h"
#include <glm/vec3.hpp>

namespace voxel {

/**
 * Simple volume implementation which stores data in a single large 3D array.
 */
class RawVolume {
public:
	class Sampler : public VolumeSampler<RawVolume> {
	private:
		using Super = VolumeSampler<RawVolume>;
	public:
		VOLUMESAMPLERUSING;

		inline bool setVoxel(const Voxel &voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			*_currentVoxel = voxel;
			return true;
		}
	};

	RawVolume(const Voxel *data, const voxel::Region &region);
	RawVolume(Voxel *data, const voxel::Region &region);

public:
	/// Constructor for creating a fixed size volume.
	RawVolume(const Region &region);
	RawVolume(const RawVolume *copy);
	RawVolume(const RawVolume &copy);
	RawVolume(RawVolume &&move) noexcept;
	RawVolume(const RawVolume &copy, const Region &region, bool *onlyAir = nullptr);
	RawVolume(const RawVolume &copy, const core::DynamicArray<Region> &regions);

	void copyInto(const RawVolume &src);

	/**
	 * @brief Calculate the amount of bytes a volume with the given region would consume
	 */
	static size_t size(const Region &region);

	static RawVolume *createRaw(const Voxel *data, const voxel::Region &region) {
		return new RawVolume(data, region);
	}

	static RawVolume *createRaw(Voxel *data, const voxel::Region &region) {
		return new RawVolume(data, region);
	}

	~RawVolume();

	/**
	 * Copy the raw data of the volume
	 * @note It's the callers responsibility to properly release the memory.
	 */
	Voxel *copyVoxels() const;

	/**
	 * The border value is returned whenever an attempt is made to read a voxel which
	 * is outside the extents of the volume.
	 * @return The value used for voxels outside of the volume
	 */
	const Voxel &borderValue() const;

	/**
	 * @return A Region representing the extent of the volume.
	 */
	const Region &region() const;

	/**
	 * @return A Region representing the extent of the volume.
	 */
	Region &region() {
		return _region;
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

	/**
	 * the vector that describes the mins value of an aabb where a voxel is set in this volume
	 * deleting a voxel afterwards might lead to invalid results
	 */
	glm::ivec3 mins() const;
	/**
	 * the vector that describes the maxs value of an aabb where a voxel is set in this volume
	 * deleting a voxel afterwards might lead to invalid results
	 */
	glm::ivec3 maxs() const;

	/**
	 * Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	 */
	const Voxel &voxel(int32_t x, int32_t y, int32_t z) const;
	/**
	 * @brief This version of the function is provided so that the wrap mode does not need
	 * to be specified as a template parameter, as it may be confusing to some users.
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	inline const Voxel &voxel(const glm::ivec3 &pos) const;

	/**
	 * Sets the value used for voxels which are outside the volume
	 */
	void setBorderValue(const Voxel &voxel);
	/**
	 * Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	 */
	bool setVoxel(int32_t x, int32_t y, int32_t z, const Voxel &voxel);
	/**
	 * Sets the voxel at the position given by a 3D vector
	 */
	bool setVoxel(const glm::ivec3 &pos, const Voxel &voxel);
	void setVoxelUnsafe(const glm::ivec3 &pos, const Voxel &voxel);

	void clear();
	void fill(const voxel::Voxel &voxel);

	inline const uint8_t *data() const {
		return (const uint8_t *)_data;
	}

	inline Voxel *voxels() const {
		return _data;
	}

	/**
	 * @brief Shift the region of the volume by the given coordinates
	 */
	void translate(const glm::ivec3 &t) {
		_region.shift(t.x, t.y, t.z);
	}

	/**
	 * @brief Move voxels in the volumes but doesn't cut of voxels, but moves them in from the other side again.
	 * @param[in] t The translation vector that might not be bigger than the volume size
	 * @return @c false if the move wasn't possible because @c t was invalid, @c true otherwise
	 */
	bool move(const glm::ivec3 &t);

private:
	void initialise(const Region &region);

	/** The size of the volume */
	Region _region;

	/** The border value */
	Voxel _borderVoxel;

	/** The voxel data */
	Voxel *_data;
};

inline const Region &RawVolume::region() const {
	return _region;
}

inline const Voxel &RawVolume::borderValue() const {
	return _borderVoxel;
}

inline int32_t RawVolume::width() const {
	return _region.getWidthInVoxels();
}

inline int32_t RawVolume::height() const {
	return _region.getHeightInVoxels();
}

inline int32_t RawVolume::depth() const {
	return _region.getDepthInVoxels();
}

inline const Voxel &RawVolume::voxel(const glm::ivec3 &pos) const {
	return voxel(pos.x, pos.y, pos.z);
}

template<class Volume>
inline bool setVoxels(Volume &volume, int x, int z, const Voxel* voxels, int amount) {
	typename Volume::Sampler sampler(volume);
	sampler.setPosition(x, 0, z);
	for (int y = 0; y < amount; ++y) {
		sampler.setVoxel(x, y, z, voxels[y]);
		sampler.movePositiveY();
	}
	return true;
}

template<class Volume>
inline bool setVoxels(Volume &volume, int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount) {
	typename Volume::Sampler sampler(volume);
	sampler.setPosition(x, y, z);
	for (int j = 0; j < nx; ++j) {
		typename Volume::Sampler sampler2(sampler);
		for (int k = 0; k < nz; ++k) {
			typename Volume::Sampler sampler3(sampler2);
			for (int ny = 0; ny < amount; ++ny) {
				sampler3.setVoxel(voxels[ny]);
				sampler3.movePositiveY();
			}
			sampler2.movePositiveZ();
		}
		sampler.movePositiveX();
	}
	return true;
}

} // namespace voxel
