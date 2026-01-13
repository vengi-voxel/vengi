/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "math/Axis.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stdint.h>

namespace voxel {

#define CAN_GO_NEG_X(val) ((val) > region.getLowerX())
#define CAN_GO_POS_X(val) ((val) < region.getUpperX())
#define CAN_GO_NEG_Y(val) ((val) > region.getLowerY())
#define CAN_GO_POS_Y(val) ((val) < region.getUpperY())
#define CAN_GO_NEG_Z(val) ((val) > region.getLowerZ())
#define CAN_GO_POS_Z(val) ((val) < region.getUpperZ())

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
		if (core_likely(this->currentPositionValid())) {
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
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 - region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1ny1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x)) {
			return *(_currentVoxel - 1);
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx0py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - 1 + region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1nx1py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - 1 + region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x - 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel - region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1ny1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py0pz() const {
		if (this->currentPositionValid()) {
			return *_currentVoxel;
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px1py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_Y(this->_posInVolume.y) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 - region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1ny1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y - 1, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x)) {
			return *(_currentVoxel + 1);
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px0py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y, this->_posInVolume.z + 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1nz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_NEG_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + region.getWidthInVoxels() - region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z - 1);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py0pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y)) {
			return *(_currentVoxel + 1 + region.getWidthInVoxels());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z);
	}

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel1px1py1pz() const {
		const Region &region = this->region();
		if (this->currentPositionValid() && CAN_GO_POS_X(this->_posInVolume.x) && CAN_GO_POS_Y(this->_posInVolume.y) &&
			CAN_GO_POS_Z(this->_posInVolume.z)) {
			return *(_currentVoxel + 1 + region.getWidthInVoxels() + region.stride());
		}
		return this->_volume->voxel(this->_posInVolume.x + 1, this->_posInVolume.y + 1, this->_posInVolume.z + 1);
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
			const glm::aligned_ivec4 localPos = _posInVolume - region.getLowerCorner4();
			const int32_t uVoxelIndex =
				localPos.x + localPos.y * _volume->width() + localPos.z * _volume->width() * _volume->height();

			_currentVoxel = _volume->voxels() + uVoxelIndex;
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
		const bool bIsOldPositionValid = currentPositionValid();

		_posInVolume.x += (int)offset;

		if (core_unlikely(region().getUpperX() < _posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel += (intptr_t)offset;
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

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel += (intptr_t)(_volume->width() * offset);
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

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel += (intptr_t)(_volume->width() * _volume->height() * offset);
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

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel -= (intptr_t)offset;
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

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel -= (intptr_t)(_volume->width() * offset);
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

		// Then we update the voxel pointer
		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			_currentVoxel -= (intptr_t)(_volume->width() * _volume->height() * offset);
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

	/** Whether the current position is inside the volume */
	uint8_t _currentPositionInvalid = 0u;
};

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

// Generic trilinear sampling
template<class Sampler>
static voxel::Voxel sampleTrilinear(Sampler &sampler, const glm::vec3 &pos) {
	const glm::ivec3 base = glm::ivec3(glm::round(pos));
	if (!sampler.setPosition(base)) {
		return voxel::Voxel();
	}

	const glm::vec3 frac = pos - glm::vec3(base);

	// Fetch 8 corner voxels via peek APIs relative to current sampler position
	const voxel::Voxel v000 = sampler.peekVoxel0px0py0pz();
	const voxel::Voxel v100 = sampler.peekVoxel1px0py0pz();
	const voxel::Voxel v010 = sampler.peekVoxel0px1py0pz();
	const voxel::Voxel v110 = sampler.peekVoxel1px1py0pz();
	const voxel::Voxel v001 = sampler.peekVoxel0px0py1pz();
	const voxel::Voxel v101 = sampler.peekVoxel1px0py1pz();
	const voxel::Voxel v011 = sampler.peekVoxel0px1py1pz();
	const voxel::Voxel v111 = sampler.peekVoxel1px1py1pz();

	// Compute trilinear weights
	const float w000 = (1.0f - frac.x) * (1.0f - frac.y) * (1.0f - frac.z);
	const float w100 = frac.x * (1.0f - frac.y) * (1.0f - frac.z);
	const float w010 = (1.0f - frac.x) * frac.y * (1.0f - frac.z);
	const float w110 = frac.x * frac.y * (1.0f - frac.z);
	const float w001 = (1.0f - frac.x) * (1.0f - frac.y) * frac.z;
	const float w101 = frac.x * (1.0f - frac.y) * frac.z;
	const float w011 = (1.0f - frac.x) * frac.y * frac.z;
	const float w111 = frac.x * frac.y * frac.z;

	// Select voxel with maximum contribution (discrete nearest in trilinear space)
	float maxWeight = w000;
	voxel::Voxel result = v000;
	if (w100 > maxWeight) {
		maxWeight = w100;
		result = v100;
	}
	if (w010 > maxWeight) {
		maxWeight = w010;
		result = v010;
	}
	if (w110 > maxWeight) {
		maxWeight = w110;
		result = v110;
	}
	if (w001 > maxWeight) {
		maxWeight = w001;
		result = v001;
	}
	if (w101 > maxWeight) {
		maxWeight = w101;
		result = v101;
	}
	if (w011 > maxWeight) {
		maxWeight = w011;
		result = v011;
	}
	if (w111 > maxWeight) {
		maxWeight = w111;
		result = v111;
	}

	return result;
}

} // namespace voxel
