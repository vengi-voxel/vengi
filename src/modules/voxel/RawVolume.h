/**
 * @file
 */

#pragma once

#include "Region.h"
#include "Voxel.h"
#include "core/collection/DynamicArray.h"
#include "math/Axis.h"
#include <glm/vec3.hpp>

namespace voxel {

/**
 * Simple volume implementation which stores data in a single large 3D array.
 */
class RawVolume {
public:
	class Sampler {
	private:
		static const uint8_t SAMPLER_INVALIDX = 1 << 0;
		static const uint8_t SAMPLER_INVALIDY = 1 << 1;
		static const uint8_t SAMPLER_INVALIDZ = 1 << 2;

	public:
		Sampler(const RawVolume &volume);
		Sampler(const RawVolume *volume);
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
		RawVolume *_volume;

		voxel::Region _region;

		// The current position in the volume
		glm::ivec3 _posInVolume{0, 0, 0};

		/** Other current position information */
		Voxel *_currentVoxel = nullptr;

		/** Whether the current position is inside the volume */
		uint8_t _currentPositionInvalid = 0u;
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

	void clear();

	inline const uint8_t *data() const {
		return (const uint8_t *)_data;
	}

	/**
	 * @brief Shift the region of the volume by the given coordinates
	 */
	void translate(const glm::ivec3 &t) {
		_region.shift(t.x, t.y, t.z);
	}

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

#define CAN_GO_NEG_X(val) ((val) > region.getLowerX())
#define CAN_GO_POS_X(val) ((val) < region.getUpperX())
#define CAN_GO_NEG_Y(val) ((val) > region.getLowerY())
#define CAN_GO_POS_Y(val) ((val) < region.getUpperY())
#define CAN_GO_NEG_Z(val) ((val) > region.getLowerZ())
#define CAN_GO_POS_Z(val) ((val) < region.getUpperZ())

inline const Region &RawVolume::Sampler::region() const {
	return _region;
}

inline const glm::ivec3 &RawVolume::Sampler::position() const {
	return _posInVolume;
}

inline const Voxel &RawVolume::Sampler::voxel() const {
	if (this->currentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline bool RawVolume::Sampler::currentPositionValid() const {
	return !_currentPositionInvalid;
}

inline bool RawVolume::Sampler::setPosition(const glm::ivec3 &v3dNewPos) {
	return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1ny1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
		CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1ny0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 - region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1ny1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
		CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx0py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx0py0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
		return *(_currentVoxel - 1);
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx0py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
		CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1py0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 + region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1nx1py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
		CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1ny1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1ny0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1ny1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px0py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px0py0pz() const {
	if (this->currentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px0py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1py0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel0px1py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1ny1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
		CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1ny0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 - region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1ny1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
		CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px0py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px0py0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
		return *(_currentVoxel + 1);
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px0py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1py1nz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
		CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + region.getWidthInVoxels() - region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1py0pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 + region.getWidthInVoxels());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel &RawVolume::Sampler::peekVoxel1px1py1pz() const {
	const Region &region = this->region();
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
		CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + region.getWidthInVoxels() + region.stride());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

} // namespace voxel
