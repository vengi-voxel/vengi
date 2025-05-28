/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
namespace voxel {
class RawVolume;
}

namespace voxelgenerator {

struct GenlandSettings {
	unsigned int seed = 0; // Seed for the random number generator
	int size = 256;		   // Size of the generated land in voxels (width and depth) - must be a power of two
	int height = 64;
	int smoothing = 1;
	int octaves = 10;		  // Number of octaves for the noise generation
	double persistence = 0.4; /**< the persistence defines how much of the amplitude will be applied to the next noise
							   * call (only makes sense if you have @c octaves > 1). The higher this value is (ranges
							   * from 0-1) the more each new octave will add to the result.
							   */
	double riverWidth = 0.02;
	core::RGBA ground{140, 125, 115};
	core::RGBA grass{72, 80, 32};
	core::RGBA grass2{68, 78, 40};
	core::RGBA water{60, 100, 120};
};

voxel::RawVolume *genland(GenlandSettings &settings);

} // namespace voxelgenerator
