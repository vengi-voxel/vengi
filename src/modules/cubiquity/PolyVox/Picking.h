#pragma once

#include "Vector.h"

namespace PolyVox {

/**
 * A structure containing the information about a picking operation
 */
struct PickResult {
	PickResult() :
			didHit(false) {
	}
	bool didHit; ///< Did the picking operation hit anything
	Vector3DInt32 hitVoxel; ///< The location of the solid voxel it hit
	Vector3DInt32 previousVoxel; ///< The location of the voxel before the one it hit
};

/// Pick the first solid voxel along a vector
template<typename VolumeType>
PickResult pickVoxel(VolumeType* volData, const Vector3DFloat& v3dStart, const Vector3DFloat& v3dDirectionAndLength, const typename VolumeType::VoxelType& emptyVoxelExample);

namespace {

/**
 * This is just an implementation class for the pickVoxel function
 *
 * It makes note of the sort of empty voxel you're looking for in the constructor.
 *
 * Each time the operator() is called:
 *  * if it's hit a voxel it sets up the result and returns false
 *  * otherwise it preps the result for the next iteration and returns true
 */
template<typename VolumeType>
class RaycastPickingFunctor {
public:
	RaycastPickingFunctor(const typename VolumeType::VoxelType& emptyVoxelExample) :
			m_emptyVoxelExample(emptyVoxelExample), m_result() {
	}

	bool operator()(const typename VolumeType::Sampler& sampler) {
		if (sampler.getVoxel() != m_emptyVoxelExample) //If we've hit something
				{
			m_result.didHit = true;
			m_result.hitVoxel = sampler.getPosition();
			return false;
		}

		m_result.previousVoxel = sampler.getPosition();

		return true;
	}
	const typename VolumeType::VoxelType& m_emptyVoxelExample;
	PickResult m_result;
};

}

/**
 * \param volData The volume to pass the ray though
 * \param v3dStart The start position in the volume
 * \param v3dDirectionAndLength The direction and length of the ray
 * \param emptyVoxelExample The value used to represent empty voxels in your volume
 *
 * \return A PickResult containing the hit information
 */
template<typename VolumeType>
PickResult pickVoxel(VolumeType* volData, const Vector3DFloat& v3dStart, const Vector3DFloat& v3dDirectionAndLength, const typename VolumeType::VoxelType& emptyVoxelExample) {
	RaycastPickingFunctor<VolumeType> functor(emptyVoxelExample);

	raycastWithDirection(volData, v3dStart, v3dDirectionAndLength, functor);

	return functor.m_result;
}

}
