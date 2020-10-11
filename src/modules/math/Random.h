/**
 * @file
 */

#pragma once

#include "core/Algorithm.h"
#include <random>
#include <algorithm>
#include <stdlib.h>

namespace math {

class Random {
public:
	Random();

	Random(unsigned int seed);

	void setSeed(unsigned int seed);

	float randomf(float min = 0.0f, float max = 1.0f) const;

	int random(int min = 0, int max = RAND_MAX) const;

	bool fithyFifthy() const;

	float randomBinomial(float max = 1.0f) const;

	template<typename I>
	I randomElement(I begin, I end) const {
		const int n = core::distance(begin, end);
		core::next(begin, random(0, n - 1));
		return begin;
	}

	unsigned int seed() const;
private:
	unsigned int _seed;
	mutable std::default_random_engine _engine;
};

inline bool Random::fithyFifthy() const {
	return randomf() >= 0.5f;
}

inline unsigned int Random::seed() const {
	return _seed;
}

}
