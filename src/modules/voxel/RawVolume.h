/**
 * @file
 */

#pragma once

#include "Region.h"
#include "Voxel.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include "VolumeSampler.h"
#include "private/LinearVolumeStorage.h"
#include <glm/ext/vector_int3.hpp>

namespace voxel {

class VolumeDataAccessor;

/**
 * Volume facade over a storage backend (currently @c LinearVolumeStorage).
 *
 * Voxel access goes through storage; samplers bind contiguous windows via
 * @c sampleData. Swapping in a chunked/disk-backed storage means replacing the
 * storage type and implementing the same voxel/setVoxel/sampleData/layout ops
 * without changing VolumeSampler.
 *
 * Prefer @c VolumeDataAccessor for dense region reads outside the sampler path.
 */
class RawVolume {
private:
	friend class RawVolumeView;
	friend class VolumeDataAccessor;
	friend class RawVolumeWrapper;
	friend class RawVolumeMoveWrapper;
public:
	class Sampler : public VolumeSampler<RawVolume> {
	private:
		using Super = VolumeSampler<RawVolume>;
	public:
		VOLUMESAMPLERUSING;

		CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const Voxel &voxel) {
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
	RawVolume &operator=(RawVolume &&move) noexcept;
	RawVolume(const RawVolume &copy, const Region &region, bool *onlyAir = nullptr);
	RawVolume(const RawVolume &copy, const core::Buffer<Region> &regions);

	bool copyInto(const RawVolume &src);
	bool copyInto(const RawVolume &src, const voxel::Region &region);

	/**
	 * @brief Calculate the amount of bytes a volume with the given region would consume
	 */
	static size_t size(const Region &region);

	/**
	 * @brief Checks if the volume is empty in the given region
	 */
	bool isEmpty(const Region &region) const;

	/**
	 * @brief Find the nearest non-air voxel behind a plane rectangle.
	 *
	 * The plane is perpendicular to @c axis at @c slice (the slice itself is excluded).
	 * @c minA/@c maxA and @c minB/@c maxB are inclusive coordinates on
	 * @c math::getPlaneAxisA(axis) and @c math::getPlaneAxisB(axis) (X-Y-Z order:
	 * X then Y for a Z plane, X then Z for a Y plane, Y then Z for an X plane).
	 * @c out must hold (maxA-minA+1)*(maxB-minB+1) voxels, row-major
	 * with A varying fastest. Existing non-air entries are left unchanged so the
	 * caller can prefill the slice.
	 */
	void fillBehindPlane(math::Axis axis, int slice, int minA, int maxA, int minB, int maxB, Voxel *out) const;

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
	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel &voxel(int32_t x, int32_t y, int32_t z) const {
		if (_region.containsPoint(x, y, z)) {
			return _storage->voxel({x - _region.getLowerX(), y - _region.getLowerY(), z - _region.getLowerZ()});
		}
		return _borderVoxel;
	}
	/**
	 * @brief This version of the function is provided so that the wrap mode does not need
	 * to be specified as a template parameter, as it may be confusing to some users.
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel &voxel(const glm::ivec3 &pos) const {
		return voxel(pos.x, pos.y, pos.z);
	}

	/**
	 * Sets the value used for voxels which are outside the volume
	 */
	void setBorderValue(const Voxel &voxel);
	/**
	 * Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	 */
	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(int32_t x, int32_t y, int32_t z, const Voxel &voxel) {
		return setVoxel(glm::ivec3(x, y, z), voxel);
	}
	/**
	 * Sets the voxel at the position given by a 3D vector
	 */
	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const glm::ivec3 &pos, const Voxel &voxel) {
		if (core_unlikely(!_region.containsPoint(pos))) {
			return false;
		}
		return _storage->setVoxel(pos - _region.getLowerCorner(), voxel);
	}
	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(int64_t idx, const Voxel &voxel) {
		return _storage->setVoxel(idx, voxel);
	}

	void clear();
	void fill(const voxel::Voxel &voxel);

	/** Allocated voxel payload in bytes, excluding allocator overhead. */
	size_t allocatedBytes() const;

	/** True if the volume has allocated storage. */
	bool valid() const;

	CORE_FORCE_INLINE Voxel *writableVoxel(const glm::ivec3 &pos) {
		return _storage->writableVoxel(pos - _region.getLowerCorner());
	}
	/**
	 * @brief Point the sampler at a contiguous storage span covering @p pos.
	 *
	 * On return @p dataLower/@p dataUpper are world coordinates of the span
	 * bounds and @p yStride/@p zStride are element strides within that buffer.
	 */
	CORE_FORCE_INLINE Voxel *sampleData(const glm::ivec3 &pos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper,
										int64_t &yStride, int64_t &zStride) const {
		Voxel *data = _storage->sampleData(pos - _region.getLowerCorner(), dataLower, dataUpper, yStride, zStride);
		const glm::ivec3 &lower = _region.getLowerCorner();
		dataLower += lower;
		dataUpper += lower;
		return data;
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

	void removeFlags(const Region &region, uint8_t flags);
	void setFlags(const Region &region, uint8_t flags);
	bool hasFlags(const Region &region, uint8_t flags) const;
	void toggleFlags(const Region &region, uint8_t flags);
	Region regionForFlag(uint8_t flag) const;

private:
	void initialise(const Region &region, bool clear = true);

	/** The size of the volume */
	Region _region;

	/** The border value */
	Voxel _borderVoxel;

	/** Voxel payload storage (owned; never null after construction). */
	LinearVolumeStorage *_storage = nullptr;
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

} // namespace voxel
