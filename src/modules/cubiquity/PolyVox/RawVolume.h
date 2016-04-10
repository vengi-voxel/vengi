#pragma once

#include "BaseVolume.h"
#include "Region.h"
#include "Vector.h"

#include <cstdlib> //For abort()
#include <limits>
#include <memory>

namespace PolyVox {

/**
 * Simple volume implementation which stores data in a single large 3D array.
 *
 * This class is less memory-efficient than the PagedVolume, but it is the simplest possible
 * volume implementation which makes it useful for debugging and getting started with PolyVox.
 */
template<typename VoxelType>
class RawVolume: public BaseVolume<VoxelType> {
public:
	//There seems to be some descrepency between Visual Studio and GCC about how the following class should be declared.
	//There is a work around (see also See http://goo.gl/qu1wn) given below which appears to work on VS2010 and GCC, but
	//which seems to cause internal compiler errors on VS2008 when building with the /Gm 'Enable Minimal Rebuild' compiler
	//option. For now it seems best to 'fix' it with the preprocessor insstead, but maybe the workaround can be reinstated
	//in the future
	//typedef Volume<VoxelType> VolumeOfVoxelType; //Workaround for GCC/VS2010 differences.
	//class Sampler : public VolumeOfVoxelType::template Sampler< RawVolume<VoxelType> >
#if defined(_MSC_VER)
	class Sampler : public BaseVolume<VoxelType>::Sampler< RawVolume<VoxelType> > //This line works on VS2010
#else
	class Sampler: public BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> > //This line works on GCC
#endif
	{
	public:
		Sampler(RawVolume<VoxelType>* volume);
		~Sampler();

		inline VoxelType getVoxel(void) const;

		bool isCurrentPositionValid(void) const;

		void setPosition(const Vector3DInt32& v3dNewPos);
		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		inline bool setVoxel(VoxelType tValue);

		void movePositiveX(void);
		void movePositiveY(void);
		void movePositiveZ(void);

		void moveNegativeX(void);
		void moveNegativeY(void);
		void moveNegativeZ(void);

		inline VoxelType peekVoxel1nx1ny1nz(void) const;
		inline VoxelType peekVoxel1nx1ny0pz(void) const;
		inline VoxelType peekVoxel1nx1ny1pz(void) const;
		inline VoxelType peekVoxel1nx0py1nz(void) const;
		inline VoxelType peekVoxel1nx0py0pz(void) const;
		inline VoxelType peekVoxel1nx0py1pz(void) const;
		inline VoxelType peekVoxel1nx1py1nz(void) const;
		inline VoxelType peekVoxel1nx1py0pz(void) const;
		inline VoxelType peekVoxel1nx1py1pz(void) const;

		inline VoxelType peekVoxel0px1ny1nz(void) const;
		inline VoxelType peekVoxel0px1ny0pz(void) const;
		inline VoxelType peekVoxel0px1ny1pz(void) const;
		inline VoxelType peekVoxel0px0py1nz(void) const;
		inline VoxelType peekVoxel0px0py0pz(void) const;
		inline VoxelType peekVoxel0px0py1pz(void) const;
		inline VoxelType peekVoxel0px1py1nz(void) const;
		inline VoxelType peekVoxel0px1py0pz(void) const;
		inline VoxelType peekVoxel0px1py1pz(void) const;

		inline VoxelType peekVoxel1px1ny1nz(void) const;
		inline VoxelType peekVoxel1px1ny0pz(void) const;
		inline VoxelType peekVoxel1px1ny1pz(void) const;
		inline VoxelType peekVoxel1px0py1nz(void) const;
		inline VoxelType peekVoxel1px0py0pz(void) const;
		inline VoxelType peekVoxel1px0py1pz(void) const;
		inline VoxelType peekVoxel1px1py1nz(void) const;
		inline VoxelType peekVoxel1px1py0pz(void) const;
		inline VoxelType peekVoxel1px1py1pz(void) const;

	private:

		//Other current position information
		VoxelType* mCurrentVoxel;

		//Whether the current position is inside the volume
		//FIXME - Replace these with flags
		bool m_bIsCurrentPositionValidInX;
		bool m_bIsCurrentPositionValidInY;
		bool m_bIsCurrentPositionValidInZ;
	};

public:
	/// Constructor for creating a fixed size volume.
	RawVolume(const Region& regValid);

	/// Destructor
	~RawVolume();

	/// Gets the value used for voxels which are outside the volume
	VoxelType getBorderValue(void) const;
	/// Gets a Region representing the extents of the Volume.
	const Region& getEnclosingRegion(void) const;

	/// Gets the width of the volume in voxels.
	int32_t getWidth(void) const;
	/// Gets the height of the volume in voxels.
	int32_t getHeight(void) const;
	/// Gets the depth of the volume in voxels.
	int32_t getDepth(void) const;

	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	VoxelType getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	VoxelType getVoxel(const Vector3DInt32& v3dPos) const;

	/// Sets the value used for voxels which are outside the volume
	void setBorderValue(const VoxelType& tBorder);
	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, VoxelType tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const Vector3DInt32& v3dPos, VoxelType tValue);

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes(void);

protected:
	/// Copy constructor
	RawVolume(const RawVolume& rhs);

	/// Assignment operator
	RawVolume& operator=(const RawVolume& rhs);

private:
	void initialise(const Region& regValidRegion);

	//The size of the volume
	Region m_regValidRegion;

	//The border value
	VoxelType m_tBorderValue;

	//The voxel data
	VoxelType* m_pData;
};

////////////////////////////////////////////////////////////////////////////////
/// This constructor creates a volume with a fixed size which is specified as a parameter.
/// \param regValid Specifies the minimum and maximum valid voxel positions.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
RawVolume<VoxelType>::RawVolume(const Region& regValid) :
		BaseVolume<VoxelType>(), m_regValidRegion(regValid), m_tBorderValue() {
	this->setBorderValue(VoxelType());

	//Create a volume of the right size.
	initialise(regValid);
}

////////////////////////////////////////////////////////////////////////////////
/// This function should never be called. Copying volumes by value would be expensive, and we want to prevent users from doing
/// it by accident (such as when passing them as paramenters to functions). That said, there are times when you really do want to
/// make a copy of a volume and in this case you should look at the Volumeresampler.
///
/// \sa VolumeResampler
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
RawVolume<VoxelType>::RawVolume(const RawVolume<VoxelType>& /*rhs*/) {
	core_assert_msg(false, "Volume copy constructor not implemented for performance reasons.");
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
RawVolume<VoxelType>::~RawVolume() {
	delete[] m_pData;
	m_pData = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// This function should never be called. Copying volumes by value would be expensive, and we want to prevent users from doing
/// it by accident (such as when passing them as paramenters to functions). That said, there are times when you really do want to
/// make a copy of a volume and in this case you should look at the Volumeresampler.
///
/// \sa VolumeResampler
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
RawVolume<VoxelType>& RawVolume<VoxelType>::operator=(const RawVolume<VoxelType>& /*rhs*/) {
	core_assert_msg(false, "Volume assignment operator not implemented for performance reasons.");
}

////////////////////////////////////////////////////////////////////////////////
/// The border value is returned whenever an attempt is made to read a voxel which
/// is outside the extents of the volume.
/// \return The value used for voxels outside of the volume
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType RawVolume<VoxelType>::getBorderValue(void) const {
	return m_tBorderValue;
}

////////////////////////////////////////////////////////////////////////////////
/// \return A Region representing the extent of the volume.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
const Region& RawVolume<VoxelType>::getEnclosingRegion(void) const {
	return m_regValidRegion;
}

////////////////////////////////////////////////////////////////////////////////
/// \return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the width is 64.
/// \sa getHeight(), getDepth()
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
int32_t RawVolume<VoxelType>::getWidth(void) const {
	return m_regValidRegion.getUpperX() - m_regValidRegion.getLowerX() + 1;
}

////////////////////////////////////////////////////////////////////////////////
/// \return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the height is 64.
/// \sa getWidth(), getDepth()
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
int32_t RawVolume<VoxelType>::getHeight(void) const {
	return m_regValidRegion.getUpperY() - m_regValidRegion.getLowerY() + 1;
}

////////////////////////////////////////////////////////////////////////////////
/// \return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g. 0 to 63 then the depth is 64.
/// \sa getWidth(), getHeight()
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
int32_t RawVolume<VoxelType>::getDepth(void) const {
	return m_regValidRegion.getUpperZ() - m_regValidRegion.getLowerZ() + 1;
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// \param uXPos The \c x position of the voxel
/// \param uYPos The \c y position of the voxel
/// \param uZPos The \c z position of the voxel
/// \return The voxel value
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType RawVolume<VoxelType>::getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const {
	if (this->m_regValidRegion.containsPoint(uXPos, uYPos, uZPos)) {
		const Region& regValidRegion = this->m_regValidRegion;
		int32_t iLocalXPos = uXPos - regValidRegion.getLowerX();
		int32_t iLocalYPos = uYPos - regValidRegion.getLowerY();
		int32_t iLocalZPos = uZPos - regValidRegion.getLowerZ();

		return m_pData[iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight()];
	}
	return m_tBorderValue;
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// \param v3dPos The 3D position of the voxel
/// \return The voxel value
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType RawVolume<VoxelType>::getVoxel(const Vector3DInt32& v3dPos) const {
	return getVoxel(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ());
}

////////////////////////////////////////////////////////////////////////////////
/// \param tBorder The value to use for voxels outside the volume.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void RawVolume<VoxelType>::setBorderValue(const VoxelType& tBorder) {
	m_tBorderValue = tBorder;
}

////////////////////////////////////////////////////////////////////////////////
/// \param uXPos the \c x position of the voxel
/// \param uYPos the \c y position of the voxel
/// \param uZPos the \c z position of the voxel
/// \param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void RawVolume<VoxelType>::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, VoxelType tValue) {
	if (!this->m_regValidRegion.containsPoint(Vector3DInt32(uXPos, uYPos, uZPos))) {
		core_assert_msg(false, "Position is outside valid region");
	}

	const Vector3DInt32& v3dLowerCorner = this->m_regValidRegion.getLowerCorner();
	int32_t iLocalXPos = uXPos - v3dLowerCorner.getX();
	int32_t iLocalYPos = uYPos - v3dLowerCorner.getY();
	int32_t iLocalZPos = uZPos - v3dLowerCorner.getZ();

	m_pData[iLocalXPos + iLocalYPos * this->getWidth() + iLocalZPos * this->getWidth() * this->getHeight()] = tValue;
}

////////////////////////////////////////////////////////////////////////////////
/// \param v3dPos the 3D position of the voxel
/// \param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void RawVolume<VoxelType>::setVoxel(const Vector3DInt32& v3dPos, VoxelType tValue) {
	setVoxel(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ(), tValue);
}

////////////////////////////////////////////////////////////////////////////////
/// This function should probably be made internal...
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void RawVolume<VoxelType>::initialise(const Region& regValidRegion) {
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
	m_pData = new VoxelType[this->getWidth() * this->getHeight() * this->getDepth()];

	// Clear to zeros
	std::fill(m_pData, m_pData + this->getWidth() * this->getHeight() * this->getDepth(), VoxelType());
}

////////////////////////////////////////////////////////////////////////////////
/// Note: This function needs reviewing for accuracy...
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
uint32_t RawVolume<VoxelType>::calculateSizeInBytes(void) {
	return this->getWidth() * this->getHeight() * this->getDepth() * sizeof(VoxelType);
}

#define CAN_GO_NEG_X(val) (val > this->mVolume->getEnclosingRegion().getLowerX())
#define CAN_GO_POS_X(val) (val < this->mVolume->getEnclosingRegion().getUpperX())
#define CAN_GO_NEG_Y(val) (val > this->mVolume->getEnclosingRegion().getLowerY())
#define CAN_GO_POS_Y(val) (val < this->mVolume->getEnclosingRegion().getUpperY())
#define CAN_GO_NEG_Z(val) (val > this->mVolume->getEnclosingRegion().getLowerZ())
#define CAN_GO_POS_Z(val) (val < this->mVolume->getEnclosingRegion().getUpperZ())

template<typename VoxelType>
RawVolume<VoxelType>::Sampler::Sampler(RawVolume<VoxelType>* volume) :
		BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >(volume), mCurrentVoxel(0), m_bIsCurrentPositionValidInX(false), m_bIsCurrentPositionValidInY(false), m_bIsCurrentPositionValidInZ(
				false) {
}

template<typename VoxelType>
RawVolume<VoxelType>::Sampler::~Sampler() {
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::getVoxel(void) const {
	if (this->isCurrentPositionValid()) {
		return *mCurrentVoxel;
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
bool inline RawVolume<VoxelType>::Sampler::isCurrentPositionValid(void) const {
	return m_bIsCurrentPositionValidInX && m_bIsCurrentPositionValidInY && m_bIsCurrentPositionValidInZ;
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::setPosition(const Vector3DInt32& v3dNewPos) {
	setPosition(v3dNewPos.getX(), v3dNewPos.getY(), v3dNewPos.getZ());
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::setPosition(xPos, yPos, zPos);

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(xPos);
	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(yPos);
	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(zPos);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid()) {
		const Vector3DInt32& v3dLowerCorner = this->mVolume->m_regValidRegion.getLowerCorner();
		const int32_t iLocalXPos = xPos - v3dLowerCorner.getX();
		const int32_t iLocalYPos = yPos - v3dLowerCorner.getY();
		const int32_t iLocalZPos = zPos - v3dLowerCorner.getZ();
		const int32_t uVoxelIndex = iLocalXPos + iLocalYPos * this->mVolume->getWidth() + iLocalZPos * this->mVolume->getWidth() * this->mVolume->getHeight();

		mCurrentVoxel = this->mVolume->m_pData + uVoxelIndex;
	} else {
		mCurrentVoxel = 0;
	}
}

template<typename VoxelType>
bool RawVolume<VoxelType>::Sampler::setVoxel(VoxelType tValue) {
	//return m_bIsCurrentPositionValid ? *mCurrentVoxel : this->mVolume->getBorderValue();
	if (this->m_bIsCurrentPositionValidInX && this->m_bIsCurrentPositionValidInY && this->m_bIsCurrentPositionValidInZ) {
		*mCurrentVoxel = tValue;
		return true;
	}
	return false;
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::movePositiveX(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::movePositiveX();

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(this->mXPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		++mCurrentVoxel;
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::movePositiveY(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::movePositiveY();

	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(this->mYPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel += this->mVolume->getWidth();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::movePositiveZ(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::movePositiveZ();

	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(this->mZPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel += this->mVolume->getWidth() * this->mVolume->getHeight();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::moveNegativeX(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::moveNegativeX();

	m_bIsCurrentPositionValidInX = this->mVolume->getEnclosingRegion().containsPointInX(this->mXPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		--mCurrentVoxel;
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::moveNegativeY(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::moveNegativeY();

	m_bIsCurrentPositionValidInY = this->mVolume->getEnclosingRegion().containsPointInY(this->mYPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel -= this->mVolume->getWidth();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void RawVolume<VoxelType>::Sampler::moveNegativeZ(void) {
	// We'll need this in a moment...
	const bool bIsOldPositionValid = this->isCurrentPositionValid();

	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<RawVolume<VoxelType> >::moveNegativeZ();

	m_bIsCurrentPositionValidInZ = this->mVolume->getEnclosingRegion().containsPointInZ(this->mZPosInVolume);

	// Then we update the voxel pointer
	if (this->isCurrentPositionValid() && bIsOldPositionValid) {
		mCurrentVoxel -= this->mVolume->getWidth() * this->mVolume->getHeight();
	} else {
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1ny1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 - this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1ny0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel - 1 - this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1ny1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 - this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx0py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx0py0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume)) {
		return *(mCurrentVoxel - 1);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx0py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 + this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1py0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel - 1 + this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1nx1py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - 1 + this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1ny1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1ny0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel - this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1ny1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px0py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px0py0pz(void) const {
	if (this->isCurrentPositionValid()) {
		return *mCurrentVoxel;
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px0py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1py0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel + this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel0px1py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1ny1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 - this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1ny0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel + 1 - this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1ny1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_NEG_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 - this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px0py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px0py0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume)) {
		return *(mCurrentVoxel + 1);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px0py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1py1nz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_NEG_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 + this->mVolume->getWidth() - this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1py0pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume)) {
		return *(mCurrentVoxel + 1 + this->mVolume->getWidth());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType RawVolume<VoxelType>::Sampler::peekVoxel1px1py1pz(void) const {
	if (this->isCurrentPositionValid() && CAN_GO_POS_X(this->mXPosInVolume) && CAN_GO_POS_Y(this->mYPosInVolume) && CAN_GO_POS_Z(this->mZPosInVolume)) {
		return *(mCurrentVoxel + 1 + this->mVolume->getWidth() + this->mVolume->getWidth() * this->mVolume->getHeight());
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

}
