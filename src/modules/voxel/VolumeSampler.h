/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/concurrent/Atomic.h"
#include "math/Axis.h"
#include "voxel/Region.h"
#include "voxel/SparseChunk.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelSampling.h"
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

template<typename Volume, bool DenseOnly = false>
class VolumeSampler {
private:
	static const uint8_t SAMPLER_INVALIDX = 1 << 0;
	static const uint8_t SAMPLER_INVALIDY = 1 << 1;
	static const uint8_t SAMPLER_INVALIDZ = 1 << 2;

	CORE_FORCE_INLINE uint32_t currentSparseGen() const {
		return _sparseGenPtr != nullptr ? (uint32_t)*_sparseGenPtr : 0u;
	}

	CORE_FORCE_INLINE void refreshSparseAtCurrent() {
		const glm::ivec3 &volLower = _volume->volumeRegion().getLowerCorner();
		_localPos.x = (int)((int64_t)_posInVolume.x - (int64_t)volLower.x);
		_localPos.y = (int)((int64_t)_posInVolume.y - (int64_t)volLower.y);
		_localPos.z = (int)((int64_t)_posInVolume.z - (int64_t)volLower.z);
		_volume->sampleSparseAt(_localPos.x, _localPos.y, _localPos.z, _sparseChunk, _sparseBrick, _sparseRow,
								_sparseWholeBrick, _sparseLower, _sparseUpper, _sparseFallback, _sparseGen);
	}

	CORE_FORCE_INLINE const Voxel &peekSparse(int dx, int dy, int dz) const {
		if (core_likely(this->currentPositionValid())) {
			if (_sparseWholeBrick && _sparseGen == currentSparseGen()) {
				const int nx = _localPos.x + dx;
				const int ny = _localPos.y + dy;
				const int nz = _localPos.z + dz;
				if (nx >= _sparseLower.x && nx <= _sparseUpper.x && ny >= _sparseLower.y && ny <= _sparseUpper.y &&
					nz >= _sparseLower.z && nz <= _sparseUpper.z) {
					if (_sparseRow == nullptr) {
						return _sparseFallback;
					}
					return _sparseRow[(_localPos.x & (SparseChunk::BrickSide - 1)) + dx + dy * SparseChunk::BrickSide +
									  dz * (SparseChunk::BrickSide * SparseChunk::BrickSide)];
				}
			}
			// Guard against signed 32-bit overflow at INT_MIN/INT_MAX
			const int64_t px = (int64_t)_posInVolume.x + dx;
			const int64_t py = (int64_t)_posInVolume.y + dy;
			const int64_t pz = (int64_t)_posInVolume.z + dz;
			if (px < (int64_t)_region.getLowerX() || px > (int64_t)_region.getUpperX() ||
				py < (int64_t)_region.getLowerY() || py > (int64_t)_region.getUpperY() ||
				pz < (int64_t)_region.getLowerZ() || pz > (int64_t)_region.getUpperZ()) {
				return _volume->borderValue();
			}
			return _volume->voxel((int32_t)px, (int32_t)py, (int32_t)pz);
		}
		const int64_t px = (int64_t)_posInVolume.x + dx;
		const int64_t py = (int64_t)_posInVolume.y + dy;
		const int64_t pz = (int64_t)_posInVolume.z + dz;
		if (px < (int64_t)_region.getLowerX() || px > (int64_t)_region.getUpperX() ||
			py < (int64_t)_region.getLowerY() || py > (int64_t)_region.getUpperY() ||
			pz < (int64_t)_region.getLowerZ() || pz > (int64_t)_region.getUpperZ()) {
			return _volume->borderValue();
		}
		return _volume->voxel((int32_t)px, (int32_t)py, (int32_t)pz);
	}

public:
	VolumeSampler(const Volume *volume) : _volume(const_cast<Volume *>(volume)), _region(volume->region()) {
		initLayout();
	}
	VolumeSampler(const Volume &volume) : _volume(const_cast<Volume *>(&volume)), _region(volume.region()) {
		initLayout();
	}
	VolumeSampler(Volume *volume) : _volume(volume), _region(volume->region()) {
		initLayout();
	}
	VolumeSampler(Volume &volume) : _volume(&volume), _region(volume.region()) {
		initLayout();
	}
	~VolumeSampler() {
	}

	CORE_FORCE_INLINE const Region &region() const {
		return _region;
	}

	CORE_FORCE_INLINE const glm::ivec3 &position() const {
		return *(const glm::ivec3 *)&_posInVolume.x;
	}

	CORE_FORCE_INLINE const Voxel &voxel() const {
		if (core_likely(this->currentPositionValid())) {
			if constexpr (DenseOnly) {
				return *_currentVoxel;
			} else {
				if (core_likely(_currentVoxel != nullptr)) {
					return *_currentVoxel;
				}
				if (core_likely(_sparseGen == currentSparseGen())) {
					if (_sparseRow != nullptr) {
						return _sparseRow[_localPos.x & (SparseChunk::BrickSide - 1)];
					}
					return _sparseFallback;
				}
				return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
			}
		}
		return this->_volume->voxel(this->_posInVolume.x, this->_posInVolume.y, this->_posInVolume.z);
	}

	CORE_FORCE_INLINE bool currentPositionValid() const {
		return !_currentPositionInvalid;
	}

	CORE_FORCE_INLINE bool setPosition(const glm::ivec3 &v3dNewPos) {
		return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
	}

#define VOLUMESAMPLER_PEEK(name, dx, dy, dz, cond, ptrOffset)                                                          \
	CORE_NO_SANITIZE_ADDRESS inline const Voxel &name() const {                                                        \
		const Region &region = this->region();                                                                         \
		if constexpr (DenseOnly) {                                                                                     \
			if (this->currentPositionValid() && (cond)) {                                                              \
				return *(_currentVoxel + (ptrOffset));                                                                 \
			}                                                                                                          \
			return this->_volume->voxel(this->_posInVolume.x + (dx), this->_posInVolume.y + (dy),                      \
										this->_posInVolume.z + (dz));                                                  \
		} else {                                                                                                       \
			if (core_likely(_currentVoxel != nullptr)) {                                                               \
				if (this->currentPositionValid() && (cond)) {                                                          \
					return *(_currentVoxel + (ptrOffset));                                                             \
				}                                                                                                      \
				return this->_volume->borderValue();                                                                   \
			}                                                                                                          \
			if (_isSparse) {                                                                                           \
				return peekSparse((dx), (dy), (dz));                                                                   \
			}                                                                                                          \
			const int64_t px = (int64_t)this->_posInVolume.x + (dx);                                                   \
			const int64_t py = (int64_t)this->_posInVolume.y + (dy);                                                   \
			const int64_t pz = (int64_t)this->_posInVolume.z + (dz);                                                   \
			if (px < (int64_t)region.getLowerX() || px > (int64_t)region.getUpperX() ||                                \
				py < (int64_t)region.getLowerY() || py > (int64_t)region.getUpperY() ||                                \
				pz < (int64_t)region.getLowerZ() || pz > (int64_t)region.getUpperZ()) {                                \
				return this->_volume->borderValue();                                                                   \
			}                                                                                                          \
			return this->_volume->voxel((int32_t)px, (int32_t)py, (int32_t)pz);                                        \
		}                                                                                                              \
	}

	VOLUMESAMPLER_PEEK(peekVoxel1nx1ny1nz, -1, -1, -1,
					   CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   -1 - _width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1nx1ny0pz, -1, -1, 0, CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y),
					   -1 - _width)
	VOLUMESAMPLER_PEEK(peekVoxel1nx1ny1pz, -1, -1, 1,
					   CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   -1 - _width + _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1nx0py1nz, -1, 0, -1, CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_NEG_Z(_posInVolume.z),
					   -1 - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1nx0py0pz, -1, 0, 0, CAN_GO_NEG_X(_posInVolume.x), -1)
	VOLUMESAMPLER_PEEK(peekVoxel1nx0py1pz, -1, 0, 1, CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_POS_Z(_posInVolume.z),
					   -1 + _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1nx1py1nz, -1, 1, -1,
					   CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   -1 + _width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1nx1py0pz, -1, 1, 0, CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y),
					   -1 + _width)
	VOLUMESAMPLER_PEEK(peekVoxel1nx1py1pz, -1, 1, 1,
					   CAN_GO_NEG_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   -1 + _width + _stride)

	VOLUMESAMPLER_PEEK(peekVoxel0px1ny1nz, 0, -1, -1, CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   -_width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel0px1ny0pz, 0, -1, 0, CAN_GO_NEG_Y(_posInVolume.y), -_width)
	VOLUMESAMPLER_PEEK(peekVoxel0px1ny1pz, 0, -1, 1, CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   -_width + _stride)
	VOLUMESAMPLER_PEEK(peekVoxel0px0py1nz, 0, 0, -1, CAN_GO_NEG_Z(_posInVolume.z), -_stride)

	CORE_NO_SANITIZE_ADDRESS inline const Voxel &peekVoxel0px0py0pz() const {
		return voxel();
	}

	VOLUMESAMPLER_PEEK(peekVoxel0px0py1pz, 0, 0, 1, CAN_GO_POS_Z(_posInVolume.z), _stride)
	VOLUMESAMPLER_PEEK(peekVoxel0px1py1nz, 0, 1, -1, CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   _width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel0px1py0pz, 0, 1, 0, CAN_GO_POS_Y(_posInVolume.y), _width)
	VOLUMESAMPLER_PEEK(peekVoxel0px1py1pz, 0, 1, 1, CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   _width + _stride)

	VOLUMESAMPLER_PEEK(peekVoxel1px1ny1nz, 1, -1, -1,
					   CAN_GO_POS_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   1 - _width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1px1ny0pz, 1, -1, 0, CAN_GO_POS_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y),
					   1 - _width)
	VOLUMESAMPLER_PEEK(peekVoxel1px1ny1pz, 1, -1, 1,
					   CAN_GO_POS_X(_posInVolume.x) && CAN_GO_NEG_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   1 - _width + _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1px0py1nz, 1, 0, -1, CAN_GO_POS_X(_posInVolume.x) && CAN_GO_NEG_Z(_posInVolume.z),
					   1 - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1px0py0pz, 1, 0, 0, CAN_GO_POS_X(_posInVolume.x), 1)
	VOLUMESAMPLER_PEEK(peekVoxel1px0py1pz, 1, 0, 1, CAN_GO_POS_X(_posInVolume.x) && CAN_GO_POS_Z(_posInVolume.z),
					   1 + _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1px1py1nz, 1, 1, -1,
					   CAN_GO_POS_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_NEG_Z(_posInVolume.z),
					   1 + _width - _stride)
	VOLUMESAMPLER_PEEK(peekVoxel1px1py0pz, 1, 1, 0, CAN_GO_POS_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y),
					   1 + _width)
	VOLUMESAMPLER_PEEK(peekVoxel1px1py1pz, 1, 1, 1,
					   CAN_GO_POS_X(_posInVolume.x) && CAN_GO_POS_Y(_posInVolume.y) && CAN_GO_POS_Z(_posInVolume.z),
					   1 + _width + _stride)
#undef VOLUMESAMPLER_PEEK

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
			if constexpr (DenseOnly) {
				const glm::ivec3 &volLower = _volume->volumeRegion().getLowerCorner();
				const int64_t lx = (int64_t)xPos - (int64_t)volLower.x;
				const int64_t ly = (int64_t)yPos - (int64_t)volLower.y;
				const int64_t lz = (int64_t)zPos - (int64_t)volLower.z;
				_currentVoxel = _denseBase + lx + ly * _width + lz * _stride;
				return true;
			} else {
				if (core_likely(!_isSparse)) {
					const glm::ivec3 &volLower = _volume->volumeRegion().getLowerCorner();
					const int64_t lx = (int64_t)xPos - (int64_t)volLower.x;
					const int64_t ly = (int64_t)yPos - (int64_t)volLower.y;
					const int64_t lz = (int64_t)zPos - (int64_t)volLower.z;
					_currentVoxel = _denseBase + lx + ly * _width + lz * _stride;
					return true;
				}
				_currentVoxel = nullptr;
				refreshSparseAtCurrent();
				return true;
			}
		}
		_currentVoxel = nullptr;
		if constexpr (!DenseOnly) {
			_sparseRow = nullptr;
			_sparseWholeBrick = false;
		}
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
		const int64_t newX = (int64_t)_posInVolume.x + (int64_t)offset;
		_posInVolume.x = (int32_t)newX;

		if (core_unlikely(newX > (int64_t)region().getUpperX() || newX < (int64_t)region().getLowerX())) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel += (intptr_t)offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel += (intptr_t)offset;
			} else {
				const int nextLx = _localPos.x + (int)offset;
				if (core_likely((_sparseRow != nullptr || _sparseWholeBrick) && nextLx <= _sparseUpper.x &&
								_sparseGen == currentSparseGen())) {
					_localPos.x = nextLx;
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveY(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();
		const int64_t newY = (int64_t)_posInVolume.y + (int64_t)offset;
		_posInVolume.y = (int32_t)newY;

		if (core_unlikely(newY > (int64_t)region().getUpperY() || newY < (int64_t)region().getLowerY())) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel += (int64_t)_width * offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel += (int64_t)_width * offset;
			} else {
				const int nextLy = _localPos.y + (int)offset;
				if (core_likely(_sparseWholeBrick && nextLy <= _sparseUpper.y && _sparseGen == currentSparseGen())) {
					_localPos.y = nextLy;
					if (_sparseRow != nullptr) {
						_sparseRow += (intptr_t)offset * SparseChunk::BrickSide;
					}
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void movePositiveZ(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();
		const int64_t newZ = (int64_t)_posInVolume.z + (int64_t)offset;
		_posInVolume.z = (int32_t)newZ;

		if (core_unlikely(newZ > (int64_t)region().getUpperZ() || newZ < (int64_t)region().getLowerZ())) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel += _stride * offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel += _stride * offset;
			} else {
				const int nextLz = _localPos.z + (int)offset;
				if (core_likely(_sparseWholeBrick && nextLz <= _sparseUpper.z && _sparseGen == currentSparseGen())) {
					_localPos.z = nextLz;
					if (_sparseRow != nullptr) {
						_sparseRow += (intptr_t)offset * (SparseChunk::BrickSide * SparseChunk::BrickSide);
					}
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
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
		const int64_t newX = (int64_t)_posInVolume.x - (int64_t)offset;
		_posInVolume.x = (int32_t)newX;

		if (core_unlikely(newX < (int64_t)region().getLowerX() || newX > (int64_t)region().getUpperX())) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel -= (intptr_t)offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel -= (intptr_t)offset;
			} else {
				const int nextLx = _localPos.x - (int)offset;
				if (core_likely((_sparseRow != nullptr || _sparseWholeBrick) && nextLx >= _sparseLower.x &&
								_sparseGen == currentSparseGen())) {
					_localPos.x = nextLx;
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeY(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();
		const int64_t newY = (int64_t)_posInVolume.y - (int64_t)offset;
		_posInVolume.y = (int32_t)newY;

		if (core_unlikely(newY < (int64_t)region().getLowerY() || newY > (int64_t)region().getUpperY())) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel -= (int64_t)_width * offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel -= (int64_t)_width * offset;
			} else {
				const int nextLy = _localPos.y - (int)offset;
				if (core_likely(_sparseWholeBrick && nextLy >= _sparseLower.y && _sparseGen == currentSparseGen())) {
					_localPos.y = nextLy;
					if (_sparseRow != nullptr) {
						_sparseRow -= (intptr_t)offset * SparseChunk::BrickSide;
					}
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
		}
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS void moveNegativeZ(uint32_t offset = 1) {
		const bool bIsOldPositionValid = currentPositionValid();
		const int64_t newZ = (int64_t)_posInVolume.z - (int64_t)offset;
		_posInVolume.z = (int32_t)newZ;

		if (core_unlikely(newZ < (int64_t)region().getLowerZ() || newZ > (int64_t)region().getUpperZ())) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}

		if (core_unlikely(!bIsOldPositionValid)) {
			setPosition(_posInVolume);
		} else if (core_likely(currentPositionValid())) {
			if constexpr (DenseOnly) {
				_currentVoxel -= _stride * offset;
			} else if (core_likely(!_isSparse)) {
				_currentVoxel -= _stride * offset;
			} else {
				const int nextLz = _localPos.z - (int)offset;
				if (core_likely(_sparseWholeBrick && nextLz >= _sparseLower.z && _sparseGen == currentSparseGen())) {
					_localPos.z = nextLz;
					if (_sparseRow != nullptr) {
						_sparseRow -= (intptr_t)offset * (SparseChunk::BrickSide * SparseChunk::BrickSide);
					}
				} else {
					refreshSparseAtCurrent();
				}
			}
		} else {
			_currentVoxel = nullptr;
			if constexpr (!DenseOnly) {
				_sparseRow = nullptr;
				_sparseWholeBrick = false;
			}
		}
	}

protected:
	CORE_FORCE_INLINE void initLayout() {
		_width = _volume->width();
		_stride = _volume->volumeRegion().stride();
		_denseBase = _volume->linearVoxels();
		if constexpr (!DenseOnly) {
			_isSparse = (_denseBase == nullptr);
			_sparseGenPtr = _isSparse ? _volume->sparseGenerationPtr() : nullptr;
		}
	}

	CORE_FORCE_INLINE void writeCurrentSparse(const Voxel &v, bool overwrite) {
		if (overwrite) {
			_volume->setVoxelOverwrite(_posInVolume.x, _posInVolume.y, _posInVolume.z, v);
		} else {
			_volume->setVoxel(_posInVolume.x, _posInVolume.y, _posInVolume.z, v);
		}
		refreshSparseAtCurrent();
	}

	Volume *_volume;

	voxel::Region _region;

	// The current position in the volume
	glm::aligned_ivec4 _posInVolume{0, 0, 0, 0};

	/** Direct dense pointer (nullptr when sparse or out-of-bounds) */
	Voxel *_currentVoxel = nullptr;
	Voxel *_denseBase = nullptr;

	/** Cached volume layout for peek/move pointer arithmetic */
	int64_t _stride = 0;
	int32_t _width = 0;

	/** Whether the current position is inside the volume */
	uint8_t _currentPositionInvalid = 0u;

	/** SparseChunk row/brick cache (unused when DenseOnly == true) */
	bool _isSparse = false;
	bool _sparseWholeBrick = false;
	uint32_t _sparseGen = 0u;
	const core::AtomicInt *_sparseGenPtr = nullptr;
	glm::ivec3 _localPos{0};
	glm::ivec3 _sparseLower{0};
	glm::ivec3 _sparseUpper{0};
	const SparseChunk *_sparseChunk = nullptr;
	const SparseChunk::Brick *_sparseBrick = nullptr;
	const Voxel *_sparseRow = nullptr;
	Voxel _sparseFallback{};
};

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

} // namespace voxel
