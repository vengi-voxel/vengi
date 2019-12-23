/**
 * @file
 */
#pragma once

/**
 * @file
 * @defgroup Random
 * @{
 * Everything that uses randomness in SimpleAI should use these helper methods.
 * You can query random values in a thread safe way and can modify the seed - which
 * really might ease debugging at some points.
 */

#include <random>
#include <algorithm>
#include <iterator>
#include <stdlib.h>
#include "common/Thread.h"

namespace ai {

inline std::default_random_engine& randomEngine() {
	AI_THREAD_LOCAL std::default_random_engine engine;
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

/**
 * @brief Helper function to cut the input vector down to @c n random elements.
 */
template<typename T>
inline void randomElements(std::vector<T>& vec, int n) {
	if (n >= (int)vec.size()) {
		return;
	}
	std::shuffle(vec.begin(), vec.end(), randomEngine());
	vec.resize(n);
}

template<typename I>
inline void shuffle(I begin, I end) {
	std::shuffle(begin, end, randomEngine());
}

}

/**
 * @}
 */
