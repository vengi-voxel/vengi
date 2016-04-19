#pragma once

#include "core/Common.h"
#include "Region.h"
#include "Utility.h"
#include <limits>

namespace PolyVox {

/// The BaseVolume class provides common functionality and an interface for other volume classes to implement.
/// You should not try to create an instance of this class directly. Instead you should use RawVolume or PagedVolume.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \sa RawVolume, PagedVolume
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename _VoxelType>
class BaseVolume {
public:
	typedef _VoxelType VoxelType;

	template<typename DerivedVolumeType>
	class Sampler {
	public:
		Sampler(DerivedVolumeType* volume);
		~Sampler();

		Vector3DInt32 getPosition(void) const;
		inline VoxelType getVoxel(void) const;

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

	protected:

		DerivedVolumeType* mVolume;

		//The current position in the volume
		int32_t mXPosInVolume;
		int32_t mYPosInVolume;
		int32_t mZPosInVolume;
	};

public:
	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	VoxelType getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	VoxelType getVoxel(const Vector3DInt32& v3dPos) const;

	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, VoxelType tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const Vector3DInt32& v3dPos, VoxelType tValue);

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes(void);

protected:
	/// Constructor for creating a volume.
	BaseVolume();

	/// Copy constructor
	BaseVolume(const BaseVolume& rhs);

	/// Destructor
	~BaseVolume();

	/// Assignment operator
	BaseVolume& operator=(const BaseVolume& rhs);
};

////////////////////////////////////////////////////////////////////////////////
/// This is protected because you should never create a BaseVolume directly, you should instead use one of the derived classes.
///
/// \sa RawVolume, PagedVolume
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
BaseVolume<VoxelType>::BaseVolume() {
}

////////////////////////////////////////////////////////////////////////////////
/// This function should never be called. Copying volumes by value would be expensive, and we want to prevent users from doing
/// it by accident (such as when passing them as paramenters to functions). That said, there are times when you really do want to
/// make a copy of a volume and in this case you should look at the VolumeResampler.
///
/// \sa VolumeResampler
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
BaseVolume<VoxelType>::BaseVolume(const BaseVolume<VoxelType>& /*rhs*/) {
	core_assert_msg(false, "Volume copy constructor not implemented to prevent accidental copying.");
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
BaseVolume<VoxelType>::~BaseVolume() {
}

////////////////////////////////////////////////////////////////////////////////
/// This function should never be called. Copying volumes by value would be expensive, and we want to prevent users from doing
/// it by accident (such as when passing them as paramenters to functions). That said, there are times when you really do want to
/// make a copy of a volume and in this case you should look at the VolumeResampler.
///
/// \sa VolumeResampler
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
BaseVolume<VoxelType>& BaseVolume<VoxelType>::operator=(const BaseVolume<VoxelType>& /*rhs*/) {
	core_assert_msg(false, "Volume copy constructor not implemented to prevent accidental copying.");
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
VoxelType BaseVolume<VoxelType>::getVoxel(int32_t /*uXPos*/, int32_t /*uYPos*/, int32_t /*uZPos*/) const {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return VoxelType();
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// \param v3dPos The 3D position of the voxel
/// \return The voxel value
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType BaseVolume<VoxelType>::getVoxel(const Vector3DInt32& /*v3dPos*/) const {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return VoxelType();
}

////////////////////////////////////////////////////////////////////////////////
/// \param uXPos the \c x position of the voxel
/// \param uYPos the \c y position of the voxel
/// \param uZPos the \c z position of the voxel
/// \param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void BaseVolume<VoxelType>::setVoxel(int32_t /*uXPos*/, int32_t /*uYPos*/, int32_t /*uZPos*/, VoxelType /*tValue*/) {
	core_assert_msg(false, "You should never call the base class version of this function.");
}

////////////////////////////////////////////////////////////////////////////////
/// \param v3dPos the 3D position of the voxel
/// \param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void BaseVolume<VoxelType>::setVoxel(const Vector3DInt32& /*v3dPos*/, VoxelType /*tValue*/) {
	core_assert_msg(false, "You should never call the base class version of this function.");
}

////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
uint32_t BaseVolume<VoxelType>::calculateSizeInBytes(void) {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return 0;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::Sampler(DerivedVolumeType* volume) :
		mVolume(volume), mXPosInVolume(0), mYPosInVolume(0), mZPosInVolume(0) {
}

template<typename VoxelType>
template<typename DerivedVolumeType>
BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::~Sampler() {
}

template<typename VoxelType>
template<typename DerivedVolumeType>
Vector3DInt32 BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::getPosition(void) const {
	return Vector3DInt32(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::getVoxel(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::setPosition(const Vector3DInt32& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	mXPosInVolume = xPos;
	mYPosInVolume = yPos;
	mZPosInVolume = zPos;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
bool BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::setVoxel(VoxelType tValue) {
	return mVolume->setVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume, tValue);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::movePositiveX(void) {
	mXPosInVolume++;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::movePositiveY(void) {
	mYPosInVolume++;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::movePositiveZ(void) {
	mZPosInVolume++;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::moveNegativeX(void) {
	mXPosInVolume--;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::moveNegativeY(void) {
	mYPosInVolume--;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
void BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::moveNegativeZ(void) {
	mZPosInVolume--;
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1ny1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1ny0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1ny1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx0py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx0py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx0py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1nx1py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1ny1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1ny0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1ny1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px0py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px0py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px0py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel0px1py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1ny1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1ny0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1ny1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px0py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px0py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px0py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume + 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1py1nz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1py0pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume);
}

template<typename VoxelType>
template<typename DerivedVolumeType>
VoxelType BaseVolume<VoxelType>::Sampler<DerivedVolumeType>::peekVoxel1px1py1pz(void) const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume + 1);
}

}
