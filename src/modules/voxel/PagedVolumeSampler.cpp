/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"
#include "core/Trace.h"
#include <algorithm>

namespace voxel {

#define CAN_GO_NEG_X(val) (val > 0)
#define CAN_GO_POS_X(val) (val < this->_chunkSideLengthMinusOne)
#define CAN_GO_NEG_Y(val) (val > 0)
#define CAN_GO_POS_Y(val) (val < this->_chunkSideLengthMinusOne)
#define CAN_GO_NEG_Z(val) (val > 0)
#define CAN_GO_POS_Z(val) (val < this->_chunkSideLengthMinusOne)

#define NEG_X_DELTA (-(deltaX[this->_xPosInChunk-1]))
#define POS_X_DELTA (deltaX[this->_xPosInChunk])
#define NEG_Y_DELTA (-(deltaY[this->_yPosInChunk-1]))
#define POS_Y_DELTA (deltaY[this->_yPosInChunk])
#define NEG_Z_DELTA (-(deltaZ[this->_zPosInChunk-1]))
#define POS_Z_DELTA (deltaZ[this->_zPosInChunk])

PagedVolume::Sampler::Sampler(const PagedVolume* volume) :
		_volume(volume), _chunkSideLengthMinusOne(volume->_chunkSideLength - 1) {
}

PagedVolume::Sampler::Sampler(const PagedVolume& volume) :
		_volume(&volume), _chunkSideLengthMinusOne(volume._chunkSideLength - 1) {
}

PagedVolume::Sampler::~Sampler() {
}

const Voxel& PagedVolume::Sampler::voxelAt(int x, int y, int z) const {
	const int32_t xChunk = x >> _volume->_chunkSideLengthPower;
	const int32_t yChunk = y >> _volume->_chunkSideLengthPower;
	const int32_t zChunk = z >> _volume->_chunkSideLengthPower;
	const uint32_t xOffset = static_cast<uint32_t>(x & _volume->_chunkMask);
	const uint32_t yOffset = static_cast<uint32_t>(y & _volume->_chunkMask);
	const uint32_t zOffset = static_cast<uint32_t>(z & _volume->_chunkMask);
	if (_cachedChunk) {
		const glm::ivec3& chunkPos = _cachedChunk->chunkPos();
		if (chunkPos.x == xChunk && chunkPos.y == yChunk && chunkPos.z == zChunk) {
			return _cachedChunk->voxel(xOffset, yOffset, zOffset);
		}
	}
	_cachedChunk = _volume->chunk(xChunk, yChunk, zChunk);
	return _cachedChunk->voxel(xOffset, yOffset, zOffset);
}

void PagedVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	core_trace_scoped(SetSamplerPosition);

	// Then we update the voxel pointer
	const int32_t xChunk = xPos >> _volume->_chunkSideLengthPower;
	const int32_t yChunk = yPos >> _volume->_chunkSideLengthPower;
	const int32_t zChunk = zPos >> _volume->_chunkSideLengthPower;

	if (_currentVoxel == nullptr || _lastXChunk != xChunk || _lastYChunk != yChunk || _lastZChunk != zChunk) {
		if (_cachedChunk) {
			const glm::ivec3& chunkPos = _cachedChunk->chunkPos();
			if (chunkPos.x == xChunk && chunkPos.y == yChunk && chunkPos.z == zChunk) {
				std::swap(_cachedChunk, _currentChunk);
			} else {
				_cachedChunk = _currentChunk;
				_currentChunk = _volume->chunk(xChunk, yChunk, zChunk);
			}
		} else {
			_cachedChunk = _currentChunk;
			_currentChunk = _volume->chunk(xChunk, yChunk, zChunk);
		}
		_lastXChunk = xChunk;
		_lastYChunk = yChunk;
		_lastZChunk = zChunk;
	}

	_xPosInVolume = xPos;
	_yPosInVolume = yPos;
	_zPosInVolume = zPos;

	_xPosInChunk = static_cast<uint32_t>(xPos & _volume->_chunkMask);
	_yPosInChunk = static_cast<uint32_t>(yPos & _volume->_chunkMask);
	_zPosInChunk = static_cast<uint32_t>(zPos & _volume->_chunkMask);

	const uint32_t voxelIndexInChunk = morton256_x[_xPosInChunk] | morton256_y[_yPosInChunk] | morton256_z[_zPosInChunk];
	_currentVoxel = _currentChunk->_data + voxelIndexInChunk;
}

bool PagedVolume::Sampler::setVoxel(const Voxel& voxel) {
	if (_currentVoxel == nullptr) {
		return false;
	}
	//Need to think what effect this has on any existing iterators.
	//core_assert_msg(false, "This function cannot be used on PagedVolume samplers.");
	//TODO: the region is not updated properly - but we might not need this for paged volumes.
	*_currentVoxel = voxel;
	return true;
}

void PagedVolume::Sampler::movePositiveX() {
	_xPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_X(_xPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_X_DELTA;
		_xPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::movePositiveY() {
	_yPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_Y(_yPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_Y_DELTA;
		_yPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::movePositiveZ() {
	_zPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_Z(_zPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_Z_DELTA;
		_zPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeX() {
	_xPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_X(_xPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_X_DELTA;
		_xPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeY() {
	_yPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Y(_yPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_Y_DELTA;
		_yPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeZ() {
	_zPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Z(_zPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_Z_DELTA;
		_zPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

#undef NEG_X_DELTA
#undef POS_X_DELTA
#undef NEG_Y_DELTA
#undef POS_Y_DELTA
#undef NEG_Z_DELTA
#undef POS_Z_DELTA

}
