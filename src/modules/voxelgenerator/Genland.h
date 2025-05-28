/**
 * @file
 */

#pragma once

namespace voxel {
class RawVolume;
}

namespace voxelgenerator {

struct GenlandSettings {
	unsigned int seed = 0; // Seed for the random number generator
	int height = 64;
	int octaves = 10;		  // Number of octaves for the noise generation
	double persistence = 0.4; /**< the persistence defines how much of the amplitude will be applied to the next noise
							   * call (only makes sense if you have @c octaves > 1). The higher this value is (ranges
							   * from 0-1) the more each new octave will add to the result.
							   */
	double riverWidth = 0.02;
};

voxel::RawVolume *genland(GenlandSettings &settings);

} // namespace voxelgenerator
