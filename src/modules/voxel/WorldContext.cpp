#include "WorldContext.h"

namespace voxel {

WorldContext::WorldContext() :
	landscapeNoiseOctaves(1), landscapeNoisePersistence(0.1f), landscapeNoiseFrequency(0.005f), landscapeNoiseAmplitude(0.6f),
	caveNoiseOctaves(1), caveNoisePersistence(0.1f), caveNoiseFrequency(0.05f), caveNoiseAmplitude(0.1f), caveDensityThreshold(0.83f),
	mountainNoiseOctaves(2), mountainNoisePersistence(0.3f), mountainNoiseFrequency(0.00075f), mountainNoiseAmplitude(0.5f) {
}

}
