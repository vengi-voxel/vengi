/**
 * @file
 */

#include "Random.h"
#include "core/Assert.h"
#include <random>
#include <stdlib.h>

namespace math {

Random::Random() :
		Random(0) {
}

Random::Random(unsigned int seed) :
		_engine(std::default_random_engine()) {
	setSeed(seed);
}

void Random::setSeed(unsigned int seed) {
	_engine.seed(seed);
	_seed = seed;
}

float Random::randomf(float min, float max) const {
	core_assert(min <= max);
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(_engine);
}

int Random::random(int min, int max) const {
	core_assert(min <= max);
	if (min == max) {
		return min;
	}
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(_engine);
}

float Random::randomBinomial(float max) const {
	return this->randomf(0.0f, max) - this->randomf(0.0f, max);
}

}
