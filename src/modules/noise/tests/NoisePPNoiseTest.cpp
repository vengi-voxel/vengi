#include <gtest/gtest.h>
#include "noise/NoisePPNoise.h"
#include "NoiseTestHelper.h"

namespace noise {

TEST(NoisePPNoiseTest, testNoise) {
	NoisePPNoise noise;
	noise.setSeed(1L);
	const int worldHeight = 32;
	const int worldSize = 32;
	noise.init();
	printNoise(noise, worldSize, worldHeight, worldSize);
}

}
