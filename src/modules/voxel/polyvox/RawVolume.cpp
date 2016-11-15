/**
 * @file
 */

#include "RawVolume.h"

namespace voxel {

RawVolume::RawVolume(const Region& regValid) :
		_region(regValid) {
	setBorderValue(Voxel());
	//Create a volume of the right size.
	initialise(regValid);
}

RawVolume::RawVolume(const RawVolume* copy) :
		_region(copy->getRegion()) {
	setBorderValue(copy->getBorderValue());
	_data = new Voxel[getWidth() * getHeight() * getDepth()];
	memcpy(_data, copy->_data, calculateSizeInBytes());
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume
////////////////////////////////////////////////////////////////////////////////
RawVolume::~RawVolume() {
	delete[] _data;
	_data = 0;
}

/**
 * The border value is returned whenever an attempt is made to read a voxel which
 * is outside the extents of the volume.
 * @return The value used for voxels outside of the volume
 */
const Voxel& RawVolume::getBorderValue() const {
	return _borderVoxel;
}

/**
 * @return A Region representing the extent of the volume.
 */
const Region& RawVolume::getRegion() const {
	return _region;
}

/**
 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the width is 64.
 * @sa getHeight(), getDepth()
 */
int32_t RawVolume::getWidth() const {
	return _region.getUpperX() - _region.getLowerX() + 1;
}

/**
 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the height is 64.
 * @sa getWidth(), getDepth()
 */
int32_t RawVolume::getHeight() const {
	return _region.getUpperY() - _region.getLowerY() + 1;
}

/**
 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the depth is 64.
 * @sa getWidth(), getHeight()
 */
int32_t RawVolume::getDepth() const {
	return _region.getUpperZ() - _region.getLowerZ() + 1;
}

/**
 * This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param uXPos The @c x position of the voxel
 * @param uYPos The @c y position of the voxel
 * @param uZPos The @c z position of the voxel
 * @return The voxel value
 */
const Voxel& RawVolume::getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const {
	if (this->_region.containsPoint(uXPos, uYPos, uZPos)) {
		const Region& regValidRegion = this->_region;
		const int32_t iLocalXPos = uXPos - regValidRegion.getLowerX();
		const int32_t iLocalYPos = uYPos - regValidRegion.getLowerY();
		const int32_t iLocalZPos = uZPos - regValidRegion.getLowerZ();

		return _data[iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight()];
	}
	return _borderVoxel;
}

/**
 * @param tBorder The value to use for voxels outside the volume.
 */
void RawVolume::setBorderValue(const Voxel& tBorder) {
	_borderVoxel = tBorder;
}

/**
 * @param uXPos the @c x position of the voxel
 * @param uYPos the @c y position of the voxel
 * @param uZPos the @c z position of the voxel
 * @param tValue the value to which the voxel will be set
 * @return @c true if the voxel was placed, @c false if it was already the same voxel
 */
bool RawVolume::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue) {
	core_assert_msg(this->_region.containsPoint(glm::ivec3(uXPos, uYPos, uZPos)), "Position is outside valid region %i:%i:%i",
			uXPos, uYPos, uZPos);

	const glm::ivec3& v3dLowerCorner = this->_region.getLowerCorner();
	const int32_t iLocalXPos = uXPos - v3dLowerCorner.x;
	const int32_t iLocalYPos = uYPos - v3dLowerCorner.y;
	const int32_t iLocalZPos = uZPos - v3dLowerCorner.z;
	const int index = iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight();
	if (_data[index] == tValue) {
		return false;
	}
	_data[index] = tValue;
	return true;
}

/**
 * This function should probably be made internal...
 */
void RawVolume::initialise(const Region& regValidRegion) {
	this->_region = regValidRegion;

	core_assert_msg(this->getWidth() > 0, "Volume width must be greater than zero.");
	core_assert_msg(this->getHeight() > 0, "Volume height must be greater than zero.");
	core_assert_msg(this->getDepth() > 0, "Volume depth must be greater than zero.");

	//Create the data
	_data = new Voxel[this->getWidth() * this->getHeight() * this->getDepth()];

	// Clear to zeros
	clear();
}

void RawVolume::clear() {
	std::fill(_data, _data + this->getWidth() * this->getHeight() * this->getDepth(), Voxel());
}

/**
 * @note: This function needs reviewing for accuracy...
 */
uint32_t RawVolume::calculateSizeInBytes() {
	return this->getWidth() * this->getHeight() * this->getDepth() * sizeof(Voxel);
}

RawVolume::Sampler::Sampler(const RawVolume* volume) :
		_volume(volume), _xPosInVolume(0), _yPosInVolume(0), _zPosInVolume(0), _currentVoxel(nullptr), _isCurrentPositionValidInX(
				false), _isCurrentPositionValidInY(false), _isCurrentPositionValidInZ(false) {
}

RawVolume::Sampler::~Sampler() {
}

bool RawVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_xPosInVolume = xPos;
	_yPosInVolume = yPos;
	_zPosInVolume = zPos;

	_isCurrentPositionValidInX = this->_volume->getRegion().containsPointInX(xPos);
	_isCurrentPositionValidInY = this->_volume->getRegion().containsPointInY(yPos);
	_isCurrentPositionValidInZ = this->_volume->getRegion().containsPointInZ(zPos);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid()) {
		const glm::ivec3& v3dLowerCorner = this->_volume->_region.getLowerCorner();
		const int32_t iLocalXPos = xPos - v3dLowerCorner.x;
		const int32_t iLocalYPos = yPos - v3dLowerCorner.y;
		const int32_t iLocalZPos = zPos - v3dLowerCorner.z;
		const int32_t uVoxelIndex = iLocalXPos + iLocalYPos * this->_volume->getWidth() + iLocalZPos * this->_volume->getWidth() * this->_volume->getHeight();

		_currentVoxel = this->_volume->_data + uVoxelIndex;
		return true;
	}
	_currentVoxel = nullptr;
	return false;
}

void RawVolume::Sampler::movePositiveX() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_xPosInVolume++;

	_isCurrentPositionValidInX = this->_volume->getRegion().containsPointInX(this->_xPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		++_currentVoxel;
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

void RawVolume::Sampler::movePositiveY() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_yPosInVolume++;

	_isCurrentPositionValidInY = this->_volume->getRegion().containsPointInY(this->_yPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		_currentVoxel += this->_volume->getWidth();
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

void RawVolume::Sampler::movePositiveZ() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_zPosInVolume++;

	_isCurrentPositionValidInZ = this->_volume->getRegion().containsPointInZ(this->_zPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		_currentVoxel += this->_volume->getWidth() * this->_volume->getHeight();
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeX() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_xPosInVolume--;

	_isCurrentPositionValidInX = this->_volume->getRegion().containsPointInX(this->_xPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		--_currentVoxel;
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeY() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_yPosInVolume--;

	_isCurrentPositionValidInY = this->_volume->getRegion().containsPointInY(this->_yPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		_currentVoxel -= this->_volume->getWidth();
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeZ() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	_zPosInVolume--;

	_isCurrentPositionValidInZ = this->_volume->getRegion().containsPointInZ(this->_zPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		_currentVoxel -= this->_volume->getWidth() * this->_volume->getHeight();
	} else {
		setPosition(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume);
	}
}

}
