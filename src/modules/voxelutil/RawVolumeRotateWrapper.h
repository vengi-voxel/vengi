/**
 * @file
 */

#include "voxel/Region.h"
#include "voxel/RawVolume.h"
#include "math/Axis.h"

namespace voxelutil {

/**
 * @brief Class that switches the position indices to rotate a volume for reading without copying the data
 */
class RawVolumeRotateWrapper {
private:
	voxel::Region _region;
	const voxel::RawVolume* _volume;
	math::Axis _axis;
public:
	/**
	 * @param[in] volume The volume to rotate
	 * @param[in] axis If @c math::Axis::NONE is given here, no rotation will be executed. Otherwise the input
	 * volume is rotated by 90 degree
	 */
	RawVolumeRotateWrapper(const voxel::RawVolume* volume, math::Axis axis = math::Axis::Y);
	voxel::Region region() const;
	const voxel::Voxel& voxel(int x, int y, int z) const;
};

}