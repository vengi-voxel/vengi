/**
 * @file
 */

#include "Random.h"
#include "core/Assert.h"
#include <random>
#include <stdlib.h>

namespace math {

struct Random::Engine {
	std::default_random_engine engine;
};

Random::Random() : Random(0) {
}

Random::Random(unsigned int seed) : _engine(new Engine()) {
	setSeed(seed);
}

Random::Random(const Random &other) : _seed(other._seed), _engine(new Engine(*other._engine)) {
}

Random &Random::operator=(const Random &other) {
	if (this != &other) {
		_seed = other._seed;
		*_engine = *other._engine;
	}
	return *this;
}

Random::~Random() {
	delete _engine;
}

void Random::setSeed(unsigned int seed) {
	_engine->engine.seed(seed);
	_seed = seed;
}

float Random::randomf(float min, float max) const {
	core_assert(min <= max);
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(_engine->engine);
}

int Random::random(int min, int max) const {
	core_assert(min <= max);
	if (min == max) {
		return min;
	}
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(_engine->engine);
}

float Random::randomBinomial(float max) const {
	return this->randomf(0.0f, max) - this->randomf(0.0f, max);
}

}
