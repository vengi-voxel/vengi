/**
 * @file
 */

#pragma once

#include "color/RGBA.h"

#include <glm/vec2.hpp>

namespace voxel {
class RawVolume;
}

namespace voxelgenerator {

struct GenlandSettings {
	// Seed for the random number generator
	unsigned int seed = 0;
	// Size of the generated land in voxels (width and depth) - must be a power of two
	int size = 256;
	// Height of the generated land in voxels - must be less than 256
	int height = 64;
	// Number of octaves for the noise generation
	int octaves = 10;
	// Smoothing iterations to apply to the generated heightmap shadows
	int smoothing = 1;
	// the persistence defines how much of the amplitude will be applied to the next noise
	// call (only makes sense if you have @c octaves > 1). The higher this value is (ranges
	// from 0-1) the more each new octave will add to the result.
	double persistence = 0.4;
	double amplitude = 1.0;
	double riverWidth = 0.02;
	double freqGround = 9.5;
	double freqRiver = 13.2;
	color::RGBA ground{140, 125, 115};
	color::RGBA grass{72, 80, 32};
	color::RGBA grass2{68, 78, 40};
	color::RGBA water{60, 100, 120};
	// Apply shadows to the generated land
	bool shadow = true;
	// Generate a river in the land
	bool river = true;
	bool ambience = true;
	glm::ivec2 offset{0, 0};
};

voxel::RawVolume *genland(GenlandSettings &settings);

} // namespace voxelgenerator
