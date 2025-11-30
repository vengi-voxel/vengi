/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "color/RGBA.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "palette/Palette.h"
#include "voxel/Voxel.h"
#include <iomanip>

namespace voxel {

static const int VolumePrintThreshold = 20;

inline std::ostream &operator<<(::std::ostream &os, const voxel::VoxelType &dt) {
	return os << VoxelTypeStr[(int)dt];
}

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
	const palette::Palette &palette = voxel::getPalette();
	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		os << "z " << std::setw(3) << z << std::endl;
		for (int32_t y = upperY; y >= lowerY; --y) {
			os << "y " << std::setw(3) << y << ": ";
			for (int32_t x = lowerX; x <= upperX; ++x) {
				const glm::ivec3 pos(x, y, z);
				const voxel::Voxel &voxel = volume.voxel(pos);
				if (voxel.getMaterial() == VoxelType::Air) {
					os << ".";
				} else {
					const core::RGBA rgba = palette.color(voxel.getColor());
					os << core::Color::print(rgba, false).c_str();
				}
			}
			os << std::endl;
		}
		os << std::endl;
	}
	os << "]";
	return os;
}

} // namespace voxel
