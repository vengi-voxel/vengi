/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"

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
		_volume(volume), _xPosInVolume(0), _yPosInVolume(0), _zPosInVolume(0), _chunkSideLengthMinusOne(
				volume->_chunkSideLength - 1) {
}

PagedVolume::Sampler::Sampler(const PagedVolume& volume) :
		_volume(&volume), _xPosInVolume(0), _yPosInVolume(0), _zPosInVolume(0), _chunkSideLengthMinusOne(
				volume._chunkSideLength - 1) {
}

PagedVolume::Sampler::~Sampler() {
}

void PagedVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_xPosInVolume = xPos;
	_yPosInVolume = yPos;
	_zPosInVolume = zPos;

	// Then we update the voxel pointer
	const int32_t xChunk = _xPosInVolume >> _volume->_chunkSideLengthPower;
	const int32_t yChunk = _yPosInVolume >> _volume->_chunkSideLengthPower;
	const int32_t zChunk = _zPosInVolume >> _volume->_chunkSideLengthPower;

	_xPosInChunk = static_cast<uint16_t>(_xPosInVolume - (xChunk << _volume->_chunkSideLengthPower));
	_yPosInChunk = static_cast<uint16_t>(_yPosInVolume - (yChunk << _volume->_chunkSideLengthPower));
	_zPosInChunk = static_cast<uint16_t>(_zPosInVolume - (zChunk << _volume->_chunkSideLengthPower));

	const uint32_t voxelIndexInChunk = morton256_x[_xPosInChunk] | morton256_y[_yPosInChunk] | morton256_z[_zPosInChunk];

	_currentChunk = _volume->chunk(xChunk, yChunk, zChunk);
	_currentVoxel = _currentChunk->_data + voxelIndexInChunk;
}

bool PagedVolume::Sampler::setVoxel(const Voxel& tValue) {
	if (_currentVoxel == nullptr) {
		return false;
	}
	//Need to think what effect this has on any existing iterators.
	//core_assert_msg(false, "This function cannot be used on PagedVolume samplers.");
	*_currentVoxel = tValue;
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

}
