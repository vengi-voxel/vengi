#pragma once

#include <random>

namespace core {

class Random {
public:
	Random();

	Random(unsigned int seed);

	void setSeed(unsigned int seed);

	float randomf(float min = 0.0f, float max = 1.0f) const;

	int random(int min, int max) const;

	float randomBinomial(float max = 1.0f) const;

	template<typename I>
	I randomElement(I begin, I end) const {
		const int n = static_cast<int>(std::distance(begin, end));
		std::uniform_int_distribution<> dis(0, n - 1);
		std::advance(begin, dis(_engine));
		return begin;
	}
private:
	mutable std::default_random_engine _engine;
};

}
