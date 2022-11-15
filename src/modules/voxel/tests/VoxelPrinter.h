/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxel {

static const int VolumePrintThreshold = 10;

inline ::std::ostream &operator<<(::std::ostream &os, const voxel::Region &region) {
	return os << "region["
			  << "mins(" << region.getLowerCorner().x << ":" << region.getLowerCorner().y << ":"
			  << region.getLowerCorner().z << "), "
			  << "maxs(" << region.getUpperCorner().x << ":" << region.getUpperCorner().y << ":"
			  << region.getUpperCorner().z << ")"
			  << "]";
}

inline ::std::ostream &operator<<(::std::ostream &os, const voxel::Voxel &voxel) {
	return os << "voxel[" << voxel::VoxelTypeStr[(int)voxel.getMaterial()] << ", " << (int)voxel.getColor() << "]";
}

inline ::std::ostream &operator<<(::std::ostream &os, const voxel::RawVolume &volume) {
	const voxel::Region &region = volume.region();
	os << "volume[" << region;
	const int32_t lowerX = region.getLowerX();
	const int32_t lowerY = region.getLowerY();
	const int32_t lowerZ = region.getLowerZ();
	const int32_t upperX = core_min(lowerX + VolumePrintThreshold, region.getUpperX());
	const int32_t upperY = core_min(lowerY + VolumePrintThreshold, region.getUpperY());
	const int32_t upperZ = core_min(lowerZ + VolumePrintThreshold, region.getUpperZ());
	os << std::endl;
	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		os << "z: " << std::setw(3) << z << std::endl;
		for (int32_t y = lowerY; y <= upperY; ++y) {
			for (int32_t x = lowerX; x <= upperX; ++x) {
				const glm::ivec3 pos(x, y, z);
				const voxel::Voxel &voxel = volume.voxel(pos);
				os << "[" << std::setw(8) << voxel::VoxelTypeStr[(int)voxel.getMaterial()] << ", " << std::setw(3)
				   << (int)voxel.getColor() << "](x:" << std::setw(3) << x << ", y: " << std::setw(3) << y << ") ";
			}
			os << std::endl;
		}
		os << std::endl;
	}
	os << "]";
	return os;
}

} // namespace voxel
