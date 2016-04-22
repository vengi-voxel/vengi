#pragma once

#include "Voxel.h"
#include "Region.h"
#include "Utility.h"
#include <limits>

namespace voxel {

/// The BaseVolume class provides common functionality and an interface for other volume classes to implement.
/// You should not try to create an instance of this class directly. Instead you should use RawVolume or PagedVolume.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @sa RawVolume, PagedVolume
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BaseVolume {
public:
	template<typename DerivedVolumeType>
	class Sampler {
	public:
		Sampler(DerivedVolumeType* volume);
		~Sampler();

		inline const glm::ivec3& getPosition() const;
		inline const Voxel& getVoxel() const;

		inline void setPosition(const glm::ivec3& v3dNewPos);
		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		inline bool setVoxel(const Voxel& tValue);

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		inline Voxel peekVoxel1nx1ny1nz() const;
		inline Voxel peekVoxel1nx1ny0pz() const;
		inline Voxel peekVoxel1nx1ny1pz() const;
		inline Voxel peekVoxel1nx0py1nz() const;
		inline Voxel peekVoxel1nx0py0pz() const;
		inline Voxel peekVoxel1nx0py1pz() const;
		inline Voxel peekVoxel1nx1py1nz() const;
		inline Voxel peekVoxel1nx1py0pz() const;
		inline Voxel peekVoxel1nx1py1pz() const;

		inline Voxel peekVoxel0px1ny1nz() const;
		inline Voxel peekVoxel0px1ny0pz() const;
		inline Voxel peekVoxel0px1ny1pz() const;
		inline Voxel peekVoxel0px0py1nz() const;
		inline Voxel peekVoxel0px0py0pz() const;
		inline Voxel peekVoxel0px0py1pz() const;
		inline Voxel peekVoxel0px1py1nz() const;
		inline Voxel peekVoxel0px1py0pz() const;
		inline Voxel peekVoxel0px1py1pz() const;

		inline Voxel peekVoxel1px1ny1nz() const;
		inline Voxel peekVoxel1px1ny0pz() const;
		inline Voxel peekVoxel1px1ny1pz() const;
		inline Voxel peekVoxel1px0py1nz() const;
		inline Voxel peekVoxel1px0py0pz() const;
		inline Voxel peekVoxel1px0py1pz() const;
		inline Voxel peekVoxel1px1py1nz() const;
		inline Voxel peekVoxel1px1py0pz() const;
		inline Voxel peekVoxel1px1py1pz() const;

	protected:

		DerivedVolumeType* mVolume;

		//The current position in the volume
		int32_t mXPosInVolume;
		int32_t mYPosInVolume;
		int32_t mZPosInVolume;
	};

public:
	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	const Voxel& getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	const Voxel& getVoxel(const glm::ivec3& v3dPos) const;

	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const glm::ivec3& v3dPos, const Voxel& tValue);

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();

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

template<typename DerivedVolumeType>
inline const Voxel& BaseVolume::Sampler<DerivedVolumeType>::getVoxel() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline void BaseVolume::Sampler<DerivedVolumeType>::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

template<typename DerivedVolumeType>
inline bool BaseVolume::Sampler<DerivedVolumeType>::setVoxel(const Voxel& tValue) {
	return mVolume->setVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume, tValue);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1ny1nz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1ny0pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1ny1pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx0py1nz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx0py0pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx0py1pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1py1nz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1py0pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1nx1py1pz() const {
	return mVolume->getVoxel(mXPosInVolume - 1, mYPosInVolume + 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1ny1nz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1ny0pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1ny1pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px0py1nz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px0py0pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px0py1pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1py1nz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1py0pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel0px1py1pz() const {
	return mVolume->getVoxel(mXPosInVolume, mYPosInVolume + 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1ny1nz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1ny0pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1ny1pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume - 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px0py1nz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px0py0pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px0py1pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1py1nz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume - 1);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1py0pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume);
}

template<typename DerivedVolumeType>
inline Voxel BaseVolume::Sampler<DerivedVolumeType>::peekVoxel1px1py1pz() const {
	return mVolume->getVoxel(mXPosInVolume + 1, mYPosInVolume + 1, mZPosInVolume + 1);
}

template<typename DerivedVolumeType>
inline const glm::ivec3& BaseVolume::Sampler<DerivedVolumeType>::getPosition() const {
	return glm::ivec3(mXPosInVolume, mYPosInVolume, mZPosInVolume);
}

template<typename DerivedVolumeType>
BaseVolume::Sampler<DerivedVolumeType>::Sampler(DerivedVolumeType* volume) :
		mVolume(volume), mXPosInVolume(0), mYPosInVolume(0), mZPosInVolume(0) {
}

template<typename DerivedVolumeType>
BaseVolume::Sampler<DerivedVolumeType>::~Sampler() {
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	mXPosInVolume = xPos;
	mYPosInVolume = yPos;
	mZPosInVolume = zPos;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::movePositiveX() {
	mXPosInVolume++;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::movePositiveY() {
	mYPosInVolume++;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::movePositiveZ() {
	mZPosInVolume++;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::moveNegativeX() {
	mXPosInVolume--;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::moveNegativeY() {
	mYPosInVolume--;
}

template<typename DerivedVolumeType>
void BaseVolume::Sampler<DerivedVolumeType>::moveNegativeZ() {
	mZPosInVolume--;
}

}
