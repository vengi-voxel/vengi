/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"
#include "math/Axis.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stdint.h>
#include "math/Vec3i.h"

namespace voxel {

/**
 * @brief Sampler base for the sparse volumes. Handles the position and region checks - the voxel lookup is done by
 * the @c Derived class (CRTP).
 *
 * @c Derived must implement:
 * - @c void updateCurrentVoxel() - resolve @c _currentVoxel from @c _posInVolume
 * - @c template<int Axis> void moved(int oldCoord, int newCoord, int delta) - called after a move from a valid to a
 *   valid position along the given axis
 *
 * The non-inline members are explicitly instantiated in the volume implementation - the voxel module is always
 * compiled with optimizations, the modules using the samplers are not.
 */
template<class Derived, class Volume>
class SparseVolumeSampler {
private:
	static const uint8_t SAMPLER_INVALIDX = 1 << 0;
	static const uint8_t SAMPLER_INVALIDY = 1 << 1;
	static const uint8_t SAMPLER_INVALIDZ = 1 << 2;

	inline Derived &derived() {
		return *static_cast<Derived *>(this);
	}

	template<int Axis>
	void move(int delta);

protected:
	inline void invalidateSample() {
	}

	inline const Voxel &sampledVoxel() const {
		return currentPositionValid() ? _currentVoxel : _volume->voxel(_posInVolume.x, _posInVolume.y, _posInVolume.z);
	}

	// volatile so these stay 4-byte loads. A wide reload of _posInVolume does not forward from the stores in setPosition.
	void readPosition(int &x, int &y, int &z) const {
		x = *(const volatile int *)&_posInVolume.x;
		y = *(const volatile int *)&_posInVolume.y;
		z = *(const volatile int *)&_posInVolume.z;
	}

public:
	SparseVolumeSampler(const Volume &volume) : _volume(const_cast<Volume *>(&volume)) {
	}
	SparseVolumeSampler(const Volume *volume) : _volume(const_cast<Volume *>(volume)) {
	}

	inline const Region &region() const {
		return _volume->region();
	}

	inline const math::Vec3i &position() const {
		return _posInVolume;
	}

	inline bool currentPositionValid() const {
		return !_currentPositionInvalid;
	}

	inline const Voxel &voxel() const {
		return static_cast<const Derived *>(this)->sampledVoxel();
	}

	inline bool setPosition(const math::Vec3i &pos) {
		return setPosition(pos.x, pos.y, pos.z);
	}

	bool setPosition(int32_t xPos, int32_t yPos, int32_t zPos);

	void movePositiveX(uint32_t offset = 1);
	void movePositiveY(uint32_t offset = 1);
	void movePositiveZ(uint32_t offset = 1);
	void movePositive(math::Axis axis, uint32_t offset = 1);

	void moveNegativeX(uint32_t offset = 1);
	void moveNegativeY(uint32_t offset = 1);
	void moveNegativeZ(uint32_t offset = 1);
	void moveNegative(math::Axis axis, uint32_t offset = 1);

	inline const Voxel &peekVoxel0px0py0pz() const {
		return voxel();
	}

#define SPARSE_PEEK(name, dx, dy, dz)                                                                                  \
	inline const Voxel &name() const {                                                                                 \
		return _volume->voxel(_posInVolume.x + (dx), _posInVolume.y + (dy), _posInVolume.z + (dz));                    \
	}
	SPARSE_PEEK(peekVoxel1nx1ny1nz, -1, -1, -1)
	SPARSE_PEEK(peekVoxel1nx1ny0pz, -1, -1, 0)
	SPARSE_PEEK(peekVoxel1nx1ny1pz, -1, -1, 1)
	SPARSE_PEEK(peekVoxel1nx0py1nz, -1, 0, -1)
	SPARSE_PEEK(peekVoxel1nx0py0pz, -1, 0, 0)
	SPARSE_PEEK(peekVoxel1nx0py1pz, -1, 0, 1)
	SPARSE_PEEK(peekVoxel1nx1py1nz, -1, 1, -1)
	SPARSE_PEEK(peekVoxel1nx1py0pz, -1, 1, 0)
	SPARSE_PEEK(peekVoxel1nx1py1pz, -1, 1, 1)

	SPARSE_PEEK(peekVoxel0px1ny1nz, 0, -1, -1)
	SPARSE_PEEK(peekVoxel0px1ny0pz, 0, -1, 0)
	SPARSE_PEEK(peekVoxel0px1ny1pz, 0, -1, 1)
	SPARSE_PEEK(peekVoxel0px0py1nz, 0, 0, -1)
	SPARSE_PEEK(peekVoxel0px0py1pz, 0, 0, 1)
	SPARSE_PEEK(peekVoxel0px1py1nz, 0, 1, -1)
	SPARSE_PEEK(peekVoxel0px1py0pz, 0, 1, 0)
	SPARSE_PEEK(peekVoxel0px1py1pz, 0, 1, 1)

	SPARSE_PEEK(peekVoxel1px1ny1nz, 1, -1, -1)
	SPARSE_PEEK(peekVoxel1px1ny0pz, 1, -1, 0)
	SPARSE_PEEK(peekVoxel1px1ny1pz, 1, -1, 1)
	SPARSE_PEEK(peekVoxel1px0py1nz, 1, 0, -1)
	SPARSE_PEEK(peekVoxel1px0py0pz, 1, 0, 0)
	SPARSE_PEEK(peekVoxel1px0py1pz, 1, 0, 1)
	SPARSE_PEEK(peekVoxel1px1py1nz, 1, 1, -1)
	SPARSE_PEEK(peekVoxel1px1py0pz, 1, 1, 0)
	SPARSE_PEEK(peekVoxel1px1py1pz, 1, 1, 1)
#undef SPARSE_PEEK

protected:
	Volume *_volume;

	// The current position in the volume
	math::Vec3i _posInVolume{0, 0, 0};

	/** Other current position information */
	Voxel _currentVoxel;

	/** Whether the current position is inside the volume */
	uint8_t _currentPositionInvalid = 0u;
};

template<class Derived, class Volume>
bool SparseVolumeSampler<Derived, Volume>::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_posInVolume.x = xPos;
	_posInVolume.y = yPos;
	_posInVolume.z = zPos;

	const voxel::Region &region = this->region();
	_currentPositionInvalid = 0u;
	if (region.isValid()) {
		if (!region.containsPointInX(xPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		}
		if (!region.containsPointInY(yPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		}
		if (!region.containsPointInZ(zPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		}
	}

	if (currentPositionValid()) {
		derived().updateCurrentVoxel();
		return true;
	}
	derived().invalidateSample();
	return false;
}

template<class Derived, class Volume>
template<int Axis>
void SparseVolumeSampler<Derived, Volume>::move(int delta) {
	const bool oldPositionValid = currentPositionValid();
	int &coord = Axis == 0 ? _posInVolume.x : (Axis == 1 ? _posInVolume.y : _posInVolume.z);
	const int oldCoord = coord;
	const int newCoord = oldCoord + delta;
	coord = newCoord;

	const voxel::Region &region = this->region();
	if (region.isValid()) {
		const uint8_t invalidFlag = (uint8_t)(SAMPLER_INVALIDX << Axis);
		const bool inside = Axis == 0   ? region.containsPointInX(newCoord)
							: Axis == 1 ? region.containsPointInY(newCoord)
										: region.containsPointInZ(newCoord);
		if (inside) {
			_currentPositionInvalid &= ~invalidFlag;
		} else {
			_currentPositionInvalid |= invalidFlag;
		}
	}

	if (!oldPositionValid) {
		setPosition(_posInVolume);
		return;
	}
	if (!currentPositionValid()) {
		derived().invalidateSample();
		return;
	}
	derived().template moved<Axis>(oldCoord, newCoord, delta);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::movePositiveX(uint32_t offset) {
	move<0>((int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::movePositiveY(uint32_t offset) {
	move<1>((int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::movePositiveZ(uint32_t offset) {
	move<2>((int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::moveNegativeX(uint32_t offset) {
	move<0>(-(int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::moveNegativeY(uint32_t offset) {
	move<1>(-(int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::moveNegativeZ(uint32_t offset) {
	move<2>(-(int)offset);
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::movePositive(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		derived().movePositiveX(offset);
		break;
	case math::Axis::Y:
		derived().movePositiveY(offset);
		break;
	case math::Axis::Z:
		derived().movePositiveZ(offset);
		break;
	default:
		break;
	}
}

template<class Derived, class Volume>
void SparseVolumeSampler<Derived, Volume>::moveNegative(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		derived().moveNegativeX(offset);
		break;
	case math::Axis::Y:
		derived().moveNegativeY(offset);
		break;
	case math::Axis::Z:
		derived().moveNegativeZ(offset);
		break;
	default:
		break;
	}
}

} // namespace voxel
