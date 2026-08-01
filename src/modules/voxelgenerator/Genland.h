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
	// Height of the generated land in voxels (column clip / volume height)
	int height = 64;
	// Number of octaves for the noise generation
	int octaves = 10;
	// Smoothing iterations to apply to the generated heightmap shadows
	int smoothing = 1;
	// the persistence defines how much of the amplitude will be applied to the next noise
	// call (only makes sense if you have @c octaves > 1). The higher this value is (ranges
	// from 0-1) the more each new octave will add to the result.
	double persistence = 0.4;
	// How strongly noise pushes terrain up and down from baseHeight
	double amplitude = 20.0;
	// Average column height before noise is applied
	double baseHeight = 28.0;
	double riverWidth = 0.02;
	// How many rivers to attempt across the map
	int numRivers = 1;
	// Where rivers begin horizontally (0 = far left, 1 = far right)
	double riverPhase = 0.75;
	// How strongly river noise warps the river path (0 = straight, higher = more winding)
	double riverMeander = 4.0;
	double freqGround = 9.5;
	double freqRiver = 13.2;
	// Shifts grass tint blend (positive = more grass, negative = more ground)
	double grassBias = 0.0;
	color::RGBA ground{140, 125, 115};
	color::RGBA grass{72, 80, 32};
	color::RGBA grass2{68, 78, 40};
	color::RGBA water{60, 100, 120};
	// Apply shadows to the generated land
	bool shadow = true;
	// Shadow strength when a column is occluded (genland default 32)
	int shadowFactor = 32;
	// Generate rivers in the land
	bool river = true;
	bool ambience = true;
	// Ambient color scale applied to material colors
	double ambienceFactor = 0.3;
	glm::ivec2 offset{0, 0};
};

voxel::RawVolume *genland(GenlandSettings &settings);

} // namespace voxelgenerator
