/**
 * @file
 */

#include "RawVolume.h"

namespace voxel {

RawVolume::RawVolume(const Region& regValid) :
		m_regValidRegion(regValid), m_tBorderValue() {
	this->setBorderValue(Voxel());

	//Create a volume of the right size.
	initialise(regValid);
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume
////////////////////////////////////////////////////////////////////////////////
RawVolume::~RawVolume() {
	delete[] m_pData;
	m_pData = 0;
}

/**
 * The border value is returned whenever an attempt is made to read a voxel which
 * is outside the extents of the volume.
 * @return The value used for voxels outside of the volume
 */
const Voxel& RawVolume::getBorderValue() const {
	return m_tBorderValue;
}

/**
 * @return A Region representing the extent of the volume.
 */
const Region& RawVolume::getEnclosingRegion() const {
	return m_regValidRegion;
}

/**
 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the width is 64.
 * @sa getHeight(), getDepth()
 */
int32_t RawVolume::getWidth() const {
	return m_regValidRegion.getUpperX() - m_regValidRegion.getLowerX() + 1;
}

/**
 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the height is 64.
 * @sa getWidth(), getDepth()
 */
int32_t RawVolume::getHeight() const {
	return m_regValidRegion.getUpperY() - m_regValidRegion.getLowerY() + 1;
}

/**
 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the depth is 64.
 * @sa getWidth(), getHeight()
 */
int32_t RawVolume::getDepth() const {
	return m_regValidRegion.getUpperZ() - m_regValidRegion.getLowerZ() + 1;
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
	if (this->m_regValidRegion.containsPoint(uXPos, uYPos, uZPos)) {
		const Region& regValidRegion = this->m_regValidRegion;
		const int32_t iLocalXPos = uXPos - regValidRegion.getLowerX();
		const int32_t iLocalYPos = uYPos - regValidRegion.getLowerY();
		const int32_t iLocalZPos = uZPos - regValidRegion.getLowerZ();

		return m_pData[iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight()];
	}
	return m_tBorderValue;
}

/**
 * @param tBorder The value to use for voxels outside the volume.
 */
void RawVolume::setBorderValue(const Voxel& tBorder) {
	m_tBorderValue = tBorder;
}

/**
 * @param uXPos the @c x position of the voxel
 * @param uYPos the @c y position of the voxel
 * @param uZPos the @c z position of the voxel
 * @param tValue the value to which the voxel will be set
 */
void RawVolume::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue) {
	if (!this->m_regValidRegion.containsPoint(glm::ivec3(uXPos, uYPos, uZPos))) {
		core_assert_msg(false, "Position is outside valid region");
	}

	const glm::ivec3& v3dLowerCorner = this->m_regValidRegion.getLowerCorner();
	int32_t iLocalXPos = uXPos - v3dLowerCorner.x;
	int32_t iLocalYPos = uYPos - v3dLowerCorner.y;
	int32_t iLocalZPos = uZPos - v3dLowerCorner.z;

	m_pData[iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight()] = tValue;
}

/**
 * This function should probably be made internal...
 */
void RawVolume::initialise(const Region& regValidRegion) {
	this->m_regValidRegion = regValidRegion;

	if (this->getWidth() <= 0) {
		core_assert_msg(false, "Volume width must be greater than zero.");
	}
	if (this->getHeight() <= 0) {
		core_assert_msg(false, "Volume height must be greater than zero.");
	}
	if (this->getDepth() <= 0) {
		core_assert_msg(false, "Volume depth must be greater than zero.");
	}

	//Create the data
	m_pData = new Voxel[this->getWidth() * this->getHeight() * this->getDepth()];

	// Clear to zeros
	std::fill(m_pData, m_pData + this->getWidth() * this->getHeight() * this->getDepth(), Voxel());
}

/**
 * @note: This function needs reviewing for accuracy...
 */
uint32_t RawVolume::calculateSizeInBytes() {
	return this->getWidth() * this->getHeight() * this->getDepth() * sizeof(Voxel);
}

RawVolume::Sampler::Sampler(RawVolume* volume) :
		mVolume(volume), mXPosInVolume(0), mYPosInVolume(0), mZPosInVolume(0), mCurrentVoxel(nullptr), m_bIsCurrentPositionValidInX(
				false), m_bIsCurrentPositionValidInY(false), m_bIsCurrentPositionValidInZ(false) {
}

RawVolume::Sampler::~Sampler() {
}

void RawVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	mXPosInVolume = xPos;
	mYPosInVolume = yPos;
	mZPosInVolume = zPos;

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(xPos);
	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(yPos);
	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(zPos);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid()) {
		const glm::ivec3& v3dLowerCorner = this->mVolume->m_regValidRegion.getLowerCorner();
		const int32_t iLocalXPos = xPos - v3dLowerCorner.x;
		const int32_t iLocalYPos = yPos - v3dLowerCorner.y;
		const int32_t iLocalZPos = zPos - v3dLowerCorner.z;
		const int32_t uVoxelIndex = iLocalXPos + iLocalYPos * this->mVolume->getWidth() + iLocalZPos * this->mVolume->getWidth() * this->mVolume->getHeight();

		mCurrentVoxel = this->mVolume->m_pData + uVoxelIndex;
	} else {
		mCurrentVoxel = 0;
	}
}

void RawVolume::Sampler::movePositiveX() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mXPosInVolume++;

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(this->mXPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		++mCurrentVoxel;
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

void RawVolume::Sampler::movePositiveY() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mYPosInVolume++;

	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(this->mYPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel += this->mVolume->getWidth();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

void RawVolume::Sampler::movePositiveZ() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mZPosInVolume++;

	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(this->mZPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel += this->mVolume->getWidth() * this->mVolume->getHeight();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeX() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mXPosInVolume--;

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(this->mXPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		--mCurrentVoxel;
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeY() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mYPosInVolume--;

	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(this->mYPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel -= this->mVolume->getWidth();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

void RawVolume::Sampler::moveNegativeZ() {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	mZPosInVolume--;

	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(this->mZPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel -= this->mVolume->getWidth() * this->mVolume->getHeight();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

}
