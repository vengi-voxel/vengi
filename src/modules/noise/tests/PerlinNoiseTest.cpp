#include <gtest/gtest.h>
#include "noise/PerlinNoise.h"
#include "NoiseTestHelper.h"

namespace noise {

TEST(PerlinNoiseTest, testNoise) {
	PerlinNoise noise;
	noise.setSeed(1L);
	const int worldHeight = 16;
	const int worldSize = 16;
	printNoise(noise, worldSize, worldHeight, worldSize);
}

}
