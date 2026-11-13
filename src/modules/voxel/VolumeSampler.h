/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "math/Axis.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelSampling.h"
#include <glm/common.hpp>
#include <stdint.h>
#include <limits.h>
#include "core/concurrent/Atomic.h"

namespace voxel {

#define CAN_GO_NEG_X(val) ((val) > _dataLower.x)
#define CAN_GO_POS_X(val) ((val) < _dataUpper.x)
#define CAN_GO_NEG_Y(val) ((val) > _dataLower.y)
#define CAN_GO_POS_Y(val) ((val) < _dataUpper.y)
#define CAN_GO_NEG_Z(val) ((val) > _dataLower.z)
#define CAN_GO_POS_Z(val) ((val) < _dataUpper.z)

#define VOLUMESAMPLERUSING \
	using Super::VolumeSampler; \
	using Super::region; \
	using Super::position; \
	using Super::voxel; \
	using Super::currentPositionValid; \
	using Super::peekVoxel1nx1ny1nz; \
	using Super::peekVoxel1nx1ny0pz; \
	using Super::peekVoxel1nx1ny1pz; \
	using Super::peekVoxel1nx0py1nz; \
	using Super::peekVoxel1nx0py0pz; \
	using Super::peekVoxel1nx0py1pz; \
	using Super::peekVoxel1nx1py1nz; \
	using Super::peekVoxel1nx1py0pz; \
	using Super::peekVoxel1nx1py1pz; \
	using Super::peekVoxel0px1ny1nz; \
	using Super::peekVoxel0px1ny0pz; \
	using Super::peekVoxel0px1ny1pz; \
	using Super::peekVoxel0px0py1nz; \
	using Super::peekVoxel0px0py0pz; \
	using Super::peekVoxel0px0py1pz; \
	using Super::peekVoxel0px1py1nz; \
	using Super::peekVoxel0px1py0pz; \
	using Super::peekVoxel0px1py1pz; \
	using Super::peekVoxel1px1ny1nz; \
	using Super::peekVoxel1px1ny0pz; \
	using Super::peekVoxel1px1ny1pz; \
	using Super::peekVoxel1px0py1nz; \
	using Super::peekVoxel1px0py0pz; \
	using Super::peekVoxel1px0py1pz; \
	using Super::peekVoxel1px1py1nz; \
	using Super::peekVoxel1px1py0pz; \
	using Super::peekVoxel1px1py1pz; \
	using Super::setPosition; \
	using Super::movePositive; \
	using Super::moveNegative; \
	using Super::movePositiveX; \
	using Super::moveNegativeX; \
	using Super::movePositiveY; \
	using Super::moveNegativeY; \
	using Super::movePositiveZ; \
	using Super::moveNegativeZ

template<typename Volume>
class VolumeSampler {
private:
	static const uint8_t SAMPLER_INVALIDX = 1 << 0;
	static const uint8_t SAMPLER_INVALIDY = 1 << 1;
	static const uint8_t SAMPLER_INVALIDZ = 1 << 2;

public:
	VolumeSampler(const Volume *volume) : _volume(const_cast<Volume *>(volume)), _region(volume->region()) {
	}
	VolumeSampler(const Volume &volume) : _volume(const_cast<Volume *>(&volume)), _region(volume.region()) {
	}
	VolumeSampler(Volume *volume) : _volume(volume), _region(volume->region()) {
	}
	VolumeSampler(Volume &volume) : _volume(&volume), _region(volume.region()) {
	}
	~VolumeSampler() {
	}

	CORE_FORCE_INLINE const Region &region() const {
		return _region;
	}

	CORE_FORCE_INLINE const glm::ivec3 &position() const {
		return *(const glm::ivec3*)&_posInVolume.x;
	}

	CORE_FORCE_INLINE const Voxel &voxel() const {
		if (core_likely(_currentVoxel != nullptr && this->currentPositionValid())) {
			return *_currentVoxel;
		}
		if (this->currentPositionValid() && _chunkEntry) {
			if (Voxel *data = _chunkEntry->load()) {
				_currentVoxel = data + _dataOffset;
				return *_currentVoxel;
			}
			return *_defaultVoxel;
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_FORCE_INLINE bool currentPositionValid() const {
		return !_currentPositionInvalid;
	}

	CORE_FORCE_INLINE bool setPosition(const glm::ivec3 &v3dNewPos) {
		return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<-1, -1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 - _dataYStride);
		}
		return peekOutsideBlock<-1, -1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<-1, -1, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataZStride);
		}
		return peekOutsideBlock<-1, 0, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
			return *(_currentVoxel - 1);
		}
		return peekOutsideBlock<-1, 0, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataZStride);
		}
		return peekOutsideBlock<-1, 0, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<-1, 1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 + _dataYStride);
		}
		return peekOutsideBlock<-1, 1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<-1, 1, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<0, -1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - _dataYStride);
		}
		return peekOutsideBlock<0, -1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<0, -1, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataZStride);
		}
		return peekOutsideBlock<0, 0, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid()) {
			return *_currentVoxel;
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataZStride);
		}
		return peekOutsideBlock<0, 0, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<0, 1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + _dataYStride);
		}
		return peekOutsideBlock<0, 1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<0, 1, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<1, -1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 - _dataYStride);
		}
		return peekOutsideBlock<1, -1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<1, -1, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataZStride);
		}
		return peekOutsideBlock<1, 0, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
			return *(_currentVoxel + 1);
		}
		return peekOutsideBlock<1, 0, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataZStride);
		}
		return peekOutsideBlock<1, 0, 1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1nz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataYStride - _dataZStride);
		}
		return peekOutsideBlock<1, 1, -1>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py0pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 + _dataYStride);
		}
		return peekOutsideBlock<1, 1, 0>();
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1pz() const {
		if (_currentVoxel != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataYStride + _dataZStride);
		}
		return peekOutsideBlock<1, 1, 1>();
	}

	bool setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
		_posInVolume.x = xPos;
		_posInVolume.y = yPos;
		_posInVolume.z = zPos;

		const voxel::Region &region = this->region();
		_currentPositionInvalid = 0u;
		if (!region.containsPointInX(xPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		}
		if (!region.containsPointInY(yPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		}
		if (!region.containsPointInZ(zPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		}

		// Then we update the voxel pointer
		if (currentPositionValid()) {
			_currentVoxel = _volume->sampleData(position(), _dataLower, _dataUpper, _dataYStride, _dataZStride, _chunkEntry, _defaultVoxel);
			const glm::ivec3 local = position() - _dataLower;
			_dataOffset = (int64_t)local.x + (int64_t)local.y * _dataYStride + (int64_t)local.z * _dataZStride;
			_dataLower = glm::max(_dataLower, _region.getLowerCorner());
			_dataUpper = glm::min(_dataUpper, _region.getUpperCorner());
			return true;
		}
		_currentVoxel = nullptr;
		return false;
	}

	CORE_NO_SANITIZE_ADDRESS void movePositive(math::Axis axis, uint32_t offset = 1u) {
		switch (axis) {
		case math::Axis::X:
			movePositiveX(offset);
			break;
		case math::Axis::Y:
			movePositiveY(offset);
			break;
		case math::Axis::Z:
			movePositiveZ(offset);
			break;
		default:
			break;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveX(uint32_t offset = 1) {
		moveWithinBlock<0, 1>(offset);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveY(uint32_t offset = 1) {
		moveWithinBlock<1, 1>(offset);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveZ(uint32_t offset = 1) {
		moveWithinBlock<2, 1>(offset);
	}

	CORE_NO_SANITIZE_ADDRESS void moveNegative(math::Axis axis, uint32_t offset = 1u) {
		switch (axis) {
		case math::Axis::X:
			moveNegativeX(offset);
			break;
		case math::Axis::Y:
			moveNegativeY(offset);
			break;
		case math::Axis::Z:
			moveNegativeZ(offset);
			break;
		default:
			break;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeX(uint32_t offset = 1) {
		moveWithinBlock<0, -1>(offset);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeY(uint32_t offset = 1) {
		moveWithinBlock<1, -1>(offset);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeZ(uint32_t offset = 1) {
		moveWithinBlock<2, -1>(offset);
	}

private:
	template<int X, int Y, int Z>
	CORE_FORCE_INLINE const Voxel &peekOutsideBlock() const {
		const int64_t x = (int64_t)_posInVolume.x + X;
		const int64_t y = (int64_t)_posInVolume.y + Y;
		const int64_t z = (int64_t)_posInVolume.z + Z;
		if (x < INT_MIN || x > INT_MAX || y < INT_MIN || y > INT_MAX || z < INT_MIN || z > INT_MAX) {
			return _volume->borderValue();
		}
		return _volume->voxel((int32_t)x, (int32_t)y, (int32_t)z);
	}

	template<int Axis, int Direction>
	CORE_FORCE_INLINE void moveWithinBlock(uint32_t offset) {
		const int64_t next = (int64_t)_posInVolume[Axis] + Direction * (int64_t)offset;
		// Within a contiguous block, only the changed coordinate needs checking.
		// Recompute validity and storage state only at a block or volume boundary.
		if (core_likely(currentPositionValid() && next >= _dataLower[Axis] &&
					   next <= _dataUpper[Axis])) {
			_posInVolume[Axis] = (int32_t)next;
			const int64_t stride = Axis == 0 ? 1 : Axis == 1 ? _dataYStride : _dataZStride;
			const int64_t delta = Direction * (int64_t)offset * stride;
			if (_currentVoxel) {
				_currentVoxel += delta;
			} else {
				_dataOffset += delta;
			}
			return;
		}
		glm::ivec3 nextPosition = position();
		nextPosition[Axis] = (int32_t)next;
		setPosition(nextPosition);
	}

protected:
	Voxel *writableVoxel() {
		return _currentVoxel ? _currentVoxel : _volume->writableVoxel(position());
	}

	Volume *_volume;

	voxel::Region _region;

	// The current position in the volume
	glm::aligned_ivec4 _posInVolume{0, 0, 0, 0};

	/** Other current position information */
	mutable Voxel *_currentVoxel = nullptr;
	core::AtomicPtr<Voxel> *_chunkEntry = nullptr;
	const Voxel *_defaultVoxel = nullptr;
	int64_t _dataOffset = 0;
	glm::ivec3 _dataLower{0};
	glm::ivec3 _dataUpper{-1};
	int64_t _dataYStride = 0;
	int64_t _dataZStride = 0;

	/** Whether the current position is inside the volume */
	uint8_t _currentPositionInvalid = 0u;
};

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

} // namespace voxel
