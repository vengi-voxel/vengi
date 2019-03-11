/**
 * @file
 */

#pragma once

#include "Voxel.h"
#include "Region.h"
#include "Utility.h"
#include "core/NonCopyable.h"
#include <limits>
#include <memory>

namespace voxel {

/**
 * Simple volume implementation which stores data in a single large 3D array.
 *
 * This class is less memory-efficient than the PagedVolume, but it is the simplest possible
 * volume implementation which makes it useful for debugging and getting started with PolyVox.
 */
class RawVolume : public core::NonCopyable {
public:
	class Sampler {
	public:
		Sampler(const RawVolume& volume);
		Sampler(const RawVolume* volume);
		~Sampler();

		const Voxel& voxel() const;

		bool currentPositionValid() const;

		void setPosition(const glm::ivec3& pos);
		bool setPosition(int32_t x, int32_t y, int32_t z);
		bool setVoxel(const Voxel& voxel);
		const glm::ivec3& position() const;

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		const Voxel& peekVoxel1nx1ny1nz() const;
		const Voxel& peekVoxel1nx1ny0pz() const;
		const Voxel& peekVoxel1nx1ny1pz() const;
		const Voxel& peekVoxel1nx0py1nz() const;
		const Voxel& peekVoxel1nx0py0pz() const;
		const Voxel& peekVoxel1nx0py1pz() const;
		const Voxel& peekVoxel1nx1py1nz() const;
		const Voxel& peekVoxel1nx1py0pz() const;
		const Voxel& peekVoxel1nx1py1pz() const;

		const Voxel& peekVoxel0px1ny1nz() const;
		const Voxel& peekVoxel0px1ny0pz() const;
		const Voxel& peekVoxel0px1ny1pz() const;
		const Voxel& peekVoxel0px0py1nz() const;
		const Voxel& peekVoxel0px0py0pz() const;
		const Voxel& peekVoxel0px0py1pz() const;
		const Voxel& peekVoxel0px1py1nz() const;
		const Voxel& peekVoxel0px1py0pz() const;
		const Voxel& peekVoxel0px1py1pz() const;

		const Voxel& peekVoxel1px1ny1nz() const;
		const Voxel& peekVoxel1px1ny0pz() const;
		const Voxel& peekVoxel1px1ny1pz() const;
		const Voxel& peekVoxel1px0py1nz() const;
		const Voxel& peekVoxel1px0py0pz() const;
		const Voxel& peekVoxel1px0py1pz() const;
		const Voxel& peekVoxel1px1py1nz() const;
		const Voxel& peekVoxel1px1py0pz() const;
		const Voxel& peekVoxel1px1py1pz() const;

	protected:
		RawVolume* _volume;

		//The current position in the volume
		glm::ivec3 _posInVolume { 0, 0, 0 };

		/** Other current position information */
		Voxel* _currentVoxel = nullptr;

		/** Whether the current position is inside the volume */
		uint8_t _currentPositionInvalid = 0u;
	};

	class BufferedSampler : public Sampler {
	public:
		BufferedSampler(const RawVolume& volume, const Region& region = Region()) : Sampler(volume) {}
		BufferedSampler(const RawVolume* volume, const Region& region = Region()) : Sampler(volume) {}
		inline const uint8_t* data() const {
			return _volume->data();
		}
	};

public:
	/// Constructor for creating a fixed size volume.
	RawVolume(const Region& region);
	RawVolume(const RawVolume* copy);
	RawVolume(RawVolume&& move);

	/// Destructor
	~RawVolume();

	/// Gets the value used for voxels which are outside the volume
	const Voxel& borderValue() const;
	/// Gets a Region representing the extents of the Volume.
	const Region& region() const;

	/// Gets the width of the volume in voxels.
	int32_t width() const;
	/// Gets the height of the volume in voxels.
	int32_t height() const;
	/// Gets the depth of the volume in voxels.
	int32_t depth() const;

	/// the vector that describes the mins value of an aabb where a voxel is set in this volume
	/// deleting a voxel afterwards might lead to invalid results
	glm::ivec3 mins() const;
	/// the vector that describes the maxs value of an aabb where a voxel is set in this volume
	/// deleting a voxel afterwards might lead to invalid results
	glm::ivec3 maxs() const;

	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	const Voxel& voxel(int32_t x, int32_t y, int32_t z) const;
	/// Gets a voxel at the position given by a 3D vector
	inline const Voxel& voxel(const glm::ivec3& pos) const;

	/// Sets the value used for voxels which are outside the volume
	void setBorderValue(const Voxel& voxel);
	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	bool setVoxel(int32_t x, int32_t y, int32_t z, const Voxel& voxel);
	/// Sets the voxel at the position given by a 3D vector
	bool setVoxel(const glm::ivec3& pos, const Voxel& voxel);

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();

	void clear();

	inline const uint8_t* data() const {
		return (const uint8_t*)_data;
	}

private:
	void initialise(const Region& region);

	/** The size of the volume */
	Region _region;

	/** The border value */
	Voxel _borderVoxel;

	/** The voxel data */
	Voxel* _data;

	glm::ivec3 _mins;
	glm::ivec3 _maxs;
	bool _boundsValid;
};

/**
 * @return A Region representing the extent of the volume.
 */
inline const Region& RawVolume::region() const {
	return _region;
}

/**
 * The border value is returned whenever an attempt is made to read a voxel which
 * is outside the extents of the volume.
 * @return The value used for voxels outside of the volume
 */
inline const Voxel& RawVolume::borderValue() const {
	return _borderVoxel;
}

/**
 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the width is 64.
 * @sa height(), getDepth()
 */
inline int32_t RawVolume::width() const {
	return _region.getWidthInVoxels();
}

/**
 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the height is 64.
 * @sa width(), getDepth()
 */
inline int32_t RawVolume::height() const {
	return _region.getHeightInVoxels();
}

/**
 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the depth is 64.
 * @sa width(), height()
 */
inline int32_t RawVolume::depth() const {
	return _region.getDepthInVoxels();
}

inline glm::ivec3 RawVolume::mins() const {
	if (!_boundsValid) {
		return _region.getLowerCorner();
	}
	return _mins;
}

inline glm::ivec3 RawVolume::maxs() const {
	if (!_boundsValid) {
		return _region.getUpperCorner();
	}
	return _maxs;
}

/**
 * @brief This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param pos The 3D position of the voxel
 * @return The voxel value
 */
inline const Voxel& RawVolume::voxel(const glm::ivec3& pos) const {
	return voxel(pos.x, pos.y, pos.z);
}

#define CAN_GO_NEG_X(val) (val > this->_volume->region().getLowerX())
#define CAN_GO_POS_X(val) (val < this->_volume->region().getUpperX())
#define CAN_GO_NEG_Y(val) (val > this->_volume->region().getLowerY())
#define CAN_GO_POS_Y(val) (val < this->_volume->region().getUpperY())
#define CAN_GO_NEG_Z(val) (val > this->_volume->region().getLowerZ())
#define CAN_GO_POS_Z(val) (val < this->_volume->region().getUpperZ())

inline const glm::ivec3& RawVolume::Sampler::position() const {
	return _posInVolume;
}

inline const Voxel& RawVolume::Sampler::voxel() const {
	if (this->currentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline bool RawVolume::Sampler::currentPositionValid() const {
	return !_currentPositionInvalid;
}

inline void RawVolume::Sampler::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1ny1nz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1ny0pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 - this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1ny1pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx0py1nz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx0py0pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
		return *(_currentVoxel - 1);
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx0py1pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1py1nz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1py0pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 + this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1nx1py1pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1ny1nz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1ny0pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1ny1pz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px0py1nz() const {
	if (this->currentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px0py0pz() const {
	if (this->currentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px0py1pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1py1nz() const {
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1py0pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel0px1py1pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1ny1nz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1ny0pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 - this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1ny1pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px0py1nz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px0py0pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
		return *(_currentVoxel + 1);
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px0py1pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1py1nz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->width() - this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1py0pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 + this->_volume->width());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::peekVoxel1px1py1pz() const {
	if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->width() + this->_volume->width() * this->_volume->height());
	}
	return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

}
