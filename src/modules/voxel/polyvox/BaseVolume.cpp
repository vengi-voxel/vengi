#include "BaseVolume.h"

namespace voxel {

////////////////////////////////////////////////////////////////////////////////
/// This is protected because you should never create a BaseVolume directly, you should instead use one of the derived classes.
///
/// @sa RawVolume, PagedVolume
////////////////////////////////////////////////////////////////////////////////
BaseVolume::BaseVolume() {
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume
////////////////////////////////////////////////////////////////////////////////
BaseVolume::~BaseVolume() {
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// @param uXPos The @c x position of the voxel
/// @param uYPos The @c y position of the voxel
/// @param uZPos The @c z position of the voxel
/// @return The voxel value
////////////////////////////////////////////////////////////////////////////////
const Voxel& BaseVolume::getVoxel(int32_t /*uXPos*/, int32_t /*uYPos*/, int32_t /*uZPos*/) const {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return Voxel();
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// @param v3dPos The 3D position of the voxel
/// @return The voxel value
////////////////////////////////////////////////////////////////////////////////
const Voxel& BaseVolume::getVoxel(const glm::ivec3& /*v3dPos*/) const {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return Voxel();
}

////////////////////////////////////////////////////////////////////////////////
/// @param uXPos the @c x position of the voxel
/// @param uYPos the @c y position of the voxel
/// @param uZPos the @c z position of the voxel
/// @param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
void BaseVolume::setVoxel(int32_t /*uXPos*/, int32_t /*uYPos*/, int32_t /*uZPos*/, const Voxel& /*tValue*/) {
	core_assert_msg(false, "You should never call the base class version of this function.");
}

////////////////////////////////////////////////////////////////////////////////
/// @param v3dPos the 3D position of the voxel
/// @param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
void BaseVolume::setVoxel(const glm::ivec3& /*v3dPos*/, const Voxel& /*tValue*/) {
	core_assert_msg(false, "You should never call the base class version of this function.");
}

uint32_t BaseVolume::calculateSizeInBytes() {
	core_assert_msg(false, "You should never call the base class version of this function.");
	return 0;
}

}
