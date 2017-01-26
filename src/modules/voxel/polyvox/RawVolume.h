/**
 * @file
 */

#pragma once

#include "Voxel.h"
#include "Region.h"
#include "Utility.h"
#include "core/NonCopyable.h"
#include <limits>
#include <cstdlib> //For abort()
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

		inline const Voxel& getVoxel() const;

		inline bool isCurrentPositionValid() const;

		inline void setPosition(const glm::ivec3& pos);
		bool setPosition(int32_t x, int32_t y, int32_t z);
		inline bool setVoxel(const Voxel& voxel);
		glm::ivec3 getPosition() const;

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		inline Voxel peekVoxel1nx1ny1nz() const;
		inline Voxel peekVoxel1nx1ny0pz() const;
		inline Voxel peekVoxel1nx1ny1pz() const;
		inline Voxel peekVoxel1nx0py1nz() const;
		inline Voxel peekVoxel1nx0py0pz() const;
		inline Voxel peekVoxel1nx0py1pz() const;
		inline Voxel peekVoxel1nx1py1nz() const;
		inline Voxel peekVoxel1nx1py0pz() const;
		inline Voxel peekVoxel1nx1py1pz() const;

		inline Voxel peekVoxel0px1ny1nz() const;
		inline Voxel peekVoxel0px1ny0pz() const;
		inline Voxel peekVoxel0px1ny1pz() const;
		inline Voxel peekVoxel0px0py1nz() const;
		inline Voxel peekVoxel0px0py0pz() const;
		inline Voxel peekVoxel0px0py1pz() const;
		inline Voxel peekVoxel0px1py1nz() const;
		inline Voxel peekVoxel0px1py0pz() const;
		inline Voxel peekVoxel0px1py1pz() const;

		inline Voxel peekVoxel1px1ny1nz() const;
		inline Voxel peekVoxel1px1ny0pz() const;
		inline Voxel peekVoxel1px1ny1pz() const;
		inline Voxel peekVoxel1px0py1nz() const;
		inline Voxel peekVoxel1px0py0pz() const;
		inline Voxel peekVoxel1px0py1pz() const;
		inline Voxel peekVoxel1px1py1nz() const;
		inline Voxel peekVoxel1px1py0pz() const;
		inline Voxel peekVoxel1px1py1pz() const;

	private:
		RawVolume* _volume;

		//The current position in the volume
		glm::ivec3 _posInVolume { 0, 0, 0 };

		/** Other current position information */
		Voxel* _currentVoxel = nullptr;

		//Whether the current position is inside the volume
		//FIXME - Replace these with flags
		bool _isCurrentPositionValidInX = false;
		bool _isCurrentPositionValidInY = false;
		bool _isCurrentPositionValidInZ = false;
	};

public:
	/// Constructor for creating a fixed size volume.
	RawVolume(const Region& region);
	RawVolume(const RawVolume* copy);

	/// Destructor
	~RawVolume();

	/// Gets the value used for voxels which are outside the volume
	const Voxel& getBorderValue() const;
	/// Gets a Region representing the extents of the Volume.
	const Region& getRegion() const;

	/// Gets the width of the volume in voxels.
	int32_t getWidth() const;
	/// Gets the height of the volume in voxels.
	int32_t getHeight() const;
	/// Gets the depth of the volume in voxels.
	int32_t getDepth() const;

	/// the vector that describes the mins value of an aabb where a voxel is set in this volume
	/// deleting a voxel afterwards might lead to invalid results
	glm::ivec3 mins() const;
	/// the vector that describes the maxs value of an aabb where a voxel is set in this volume
	/// deleting a voxel afterwards might lead to invalid results
	glm::ivec3 maxs() const;

	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	const Voxel& getVoxel(int32_t x, int32_t y, int32_t z) const;
	/// Gets a voxel at the position given by a 3D vector
	inline const Voxel& getVoxel(const glm::ivec3& pos) const;

	/// Sets the value used for voxels which are outside the volume
	void setBorderValue(const Voxel& voxel);
	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	bool setVoxel(int32_t x, int32_t y, int32_t z, const Voxel& voxel);
	/// Sets the voxel at the position given by a 3D vector
	bool setVoxel(const glm::ivec3& pos, const Voxel& voxel);

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();

	void clear();

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
inline const Voxel& RawVolume::getVoxel(const glm::ivec3& pos) const {
	return getVoxel(pos.x, pos.y, pos.z);
}

#define CAN_GO_NEG_X(val) (val > this->_volume->getRegion().getLowerX())
#define CAN_GO_POS_X(val) (val < this->_volume->getRegion().getUpperX())
#define CAN_GO_NEG_Y(val) (val > this->_volume->getRegion().getLowerY())
#define CAN_GO_POS_Y(val) (val < this->_volume->getRegion().getUpperY())
#define CAN_GO_NEG_Z(val) (val > this->_volume->getRegion().getLowerZ())
#define CAN_GO_POS_Z(val) (val < this->_volume->getRegion().getUpperZ())

inline glm::ivec3 RawVolume::Sampler::getPosition() const {
	return glm::ivec3(_posInVolume.x, _posInVolume.y, _posInVolume.z);
}

inline const Voxel& RawVolume::Sampler::getVoxel() const {
	if (this->isCurrentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline bool RawVolume::Sampler::isCurrentPositionValid() const {
	return _isCurrentPositionValidInX && _isCurrentPositionValidInY && _isCurrentPositionValidInZ;
}

inline void RawVolume::Sampler::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1ny1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1ny0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 - this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1ny1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx0py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx0py0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
		return *(_currentVoxel - 1);
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx0py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1py0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - 1 + this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1nx1py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - 1 + this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1ny1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1ny0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel - this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1ny1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px0py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px0py0pz() const {
	if (this->isCurrentPositionValid()) {
		return *_currentVoxel;
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel0px0py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1py0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel0px1py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1ny1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1ny0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 - this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1ny1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px0py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px0py0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
		return *(_currentVoxel + 1);
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1px0py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1py1nz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->getWidth() - this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1py0pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
		return *(_currentVoxel + 1 + this->_volume->getWidth());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
}

inline Voxel RawVolume::Sampler::peekVoxel1px1py1pz() const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
		return *(_currentVoxel + 1 + this->_volume->getWidth() + this->_volume->getWidth() * this->_volume->getHeight());
	}
	return this->_volume->getVoxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

}
