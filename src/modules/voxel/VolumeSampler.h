/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "math/Axis.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelSampling.h"
#include <stdint.h>

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
		bindInitialSpan();
	}
	VolumeSampler(const Volume &volume) : _volume(const_cast<Volume *>(&volume)), _region(volume.region()) {
		bindInitialSpan();
	}
	VolumeSampler(Volume *volume) : _volume(volume), _region(volume->region()) {
		bindInitialSpan();
	}
	VolumeSampler(Volume &volume) : _volume(&volume), _region(volume.region()) {
		bindInitialSpan();
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
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_FORCE_INLINE bool currentPositionValid() const {
		return !_currentPositionInvalid;
	}

	CORE_FORCE_INLINE bool setPosition(const glm::ivec3 &v3dNewPos) {
		return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 - _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
			return *(_currentVoxel - 1);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 + _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid()) {
			return *_currentVoxel;
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 - _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
			return *(_currentVoxel + 1);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1nz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataYStride - _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py0pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 + _dataYStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1pz() const {
		if (_spanBase != nullptr && this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + _dataYStride + _dataZStride);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
	}

	/**
	 * Full-volume spans (linear storage) index like the pre-storage sampler.
	 * Partial/chunk spans go through setPositionSlow and re-sampleData on leave.
	 */
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

		if (currentPositionValid()) {
			if (core_likely(_spanIsFullRegion && _spanBase != nullptr)) {
				const glm::aligned_ivec4 localPos = _posInVolume - region.getLowerCorner4();
				_currentVoxel = _spanBase + (int64_t)localPos.x + (int64_t)localPos.y * _dataYStride +
								(int64_t)localPos.z * _dataZStride;
				return true;
			}
			return setPositionSlow(xPos, yPos, zPos);
		}
		_spanBase = nullptr;
		_spanIsFullRegion = false;
		_currentVoxel = nullptr;
		return false;
	}

private:
	/** Bind the contiguous window that covers the volume lower corner (chunk or full). */
	void bindInitialSpan() {
		if (!_region.isValid()) {
			return;
		}
		Voxel *atLower =
			_volume->sampleData(_region.getLowerCorner(), _dataLower, _dataUpper, _dataYStride, _dataZStride);
		updateSpanMetadata(atLower, _region.getLowerX(), _region.getLowerY(), _region.getLowerZ());
		_currentVoxel = nullptr;
	}

	void updateSpanMetadata(Voxel *atPos, int32_t xPos, int32_t yPos, int32_t zPos) {
		const int64_t localX = (int64_t)xPos - _dataLower.x;
		const int64_t localY = (int64_t)yPos - _dataLower.y;
		const int64_t localZ = (int64_t)zPos - _dataLower.z;
		_spanBase = atPos - (localX + localY * _dataYStride + localZ * _dataZStride);
		_spanIsFullRegion =
			(_dataLower.x == _region.getLowerX() && _dataLower.y == _region.getLowerY() &&
			 _dataLower.z == _region.getLowerZ() &&
			 (_dataUpper.x - _dataLower.x + 1) == _region.getWidthInVoxels() &&
			 (_dataUpper.y - _dataLower.y + 1) == _region.getHeightInVoxels() &&
			 (_dataUpper.z - _dataLower.z + 1) == _region.getDepthInVoxels() &&
			 _dataYStride == (int64_t)_region.getWidthInVoxels() && _dataZStride == _region.stride());
	}

	/** Cold path for partial/chunk spans: reuse AABB or re-sampleData. */
	__attribute__((noinline)) bool setPositionSlow(int32_t xPos, int32_t yPos, int32_t zPos) {
		if (_spanBase != nullptr && xPos >= _dataLower.x && xPos <= _dataUpper.x && yPos >= _dataLower.y &&
			yPos <= _dataUpper.y && zPos >= _dataLower.z && zPos <= _dataUpper.z) {
			const int64_t localX = (int64_t)xPos - _dataLower.x;
			const int64_t localY = (int64_t)yPos - _dataLower.y;
			const int64_t localZ = (int64_t)zPos - _dataLower.z;
			_currentVoxel = _spanBase + localX + localY * _dataYStride + localZ * _dataZStride;
			return true;
		}
		Voxel *atPos = _volume->sampleData(position(), _dataLower, _dataUpper, _dataYStride, _dataZStride);
		updateSpanMetadata(atPos, xPos, yPos, zPos);
		_currentVoxel = atPos;
		return true;
	}

public:

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
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.x += (int)offset;

		if (core_unlikely(region().getUpperX() < _posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.x <= _dataUpper.x)))) {
			_currentVoxel += (intptr_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveY(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.y += (int)offset;

		if (core_unlikely(region().getUpperY() < _posInVolume.y)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.y <= _dataUpper.y)))) {
			_currentVoxel += _dataYStride * (int64_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveZ(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.z += (int)offset;

		if (core_unlikely(region().getUpperZ() < _posInVolume.z)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.z <= _dataUpper.z)))) {
			_currentVoxel += _dataZStride * (int64_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
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
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.x -= (int)offset;

		if (region().getLowerX() > _posInVolume.x) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.x >= _dataLower.x)))) {
			_currentVoxel -= (intptr_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeY(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.y -= (int)offset;

		if (region().getLowerY() > _posInVolume.y) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.y >= _dataLower.y)))) {
			_currentVoxel -= _dataYStride * (int64_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeZ(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.z -= (int)offset;

		if (region().getLowerZ() > _posInVolume.z) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid() &&
							   (_spanIsFullRegion || (_spanBase != nullptr && _posInVolume.z >= _dataLower.z)))) {
			_currentVoxel -= _dataZStride * (int64_t)offset;
		} else if (currentPositionValid()) {
			setPosition(_posInVolume);
		} else {
			_currentVoxel = nullptr;
		}
	}

protected:
	Volume *_volume;

	voxel::Region _region;

	// The current position in the volume
	glm::aligned_ivec4 _posInVolume{0, 0, 0, 0};

	/** Other current position information */
	Voxel *_currentVoxel = nullptr;

	/** Base of the contiguous buffer returned by sampleData (voxel at dataLower). */
	Voxel *_spanBase = nullptr;
	glm::ivec3 _dataLower{0};
	glm::ivec3 _dataUpper{-1, -1, -1};
	int64_t _dataYStride = 0;
	int64_t _dataZStride = 0;
	/** True when sampleData reported the whole volume region (fast move/index path). */
	bool _spanIsFullRegion = false;

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
