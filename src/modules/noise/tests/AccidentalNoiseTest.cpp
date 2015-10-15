#include <gtest/gtest.h>
#include "noise/AccidentalNoise.h"
#include "NoiseTestHelper.h"

namespace noise {

TEST(AccidentalNoiseTest, testNoise) {
	AccidentalNoise noise;
	noise.setSeed(1L);
	const int worldHeight = 256;
	const int worldSize = 32;
#if 0
	anl::CArray3Dd a(worldSize, worldHeight, worldSize);
	anl::CImplicitModuleBase* m = noise.get();
	anl::SMappingRanges ranges;
	anl::map3D(anl::SEAMLESS_NONE, a, *m, ranges);
#endif
	printNoise(noise, worldSize, worldHeight, worldSize);
}

}
