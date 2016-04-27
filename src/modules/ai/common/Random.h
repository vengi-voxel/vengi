#pragma once

#include <chrono>
#include <random>
#include <stdlib.h>

namespace ai {

inline std::default_random_engine& randomEngine() {
	thread_local static std::default_random_engine engine;
	return engine;
}

inline void randomSeed (unsigned int seed) {
	randomEngine().seed(seed);
}

inline float randomf (float max = 1.0f) {
	std::uniform_real_distribution<float> distribution(0.0, max);
	return distribution(randomEngine());
}

inline int random (int min, int max) {
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(randomEngine());
}

inline float randomBinomial (float max = 1.0f) {
	return randomf(max) - randomf(max);
}

template<typename I>
inline I randomElement(I begin, I end) {
	const int n = static_cast<int>(std::distance(begin, end));
	std::uniform_int_distribution<> dis(0, n - 1);
	std::advance(begin, dis(randomEngine()));
	return begin;
}

}
