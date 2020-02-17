/**
 * @file
 */

#pragma once

#include "Map.h"
#include <unordered_set>
#include <algorithm>
#include <SDL_stdinc.h>

namespace core {

// TODO: c++14 adds a merge functionality to collections.

template<typename TYPE, class HASH = std::hash<TYPE> >
std::unordered_set<TYPE, HASH> setIntersection(const std::unordered_set<TYPE, HASH>& set1, const std::unordered_set<TYPE, HASH>& set2) {
	if (set1.size() > set2.size()) {
		return setIntersection(set2, set1);
	}

	std::unordered_set<TYPE, HASH> intersection;
	intersection.reserve(set2.size());
	for (auto const& element : set1) {
		if (set2.find(element) != set2.end()) {
			intersection.insert(element);
		}
	}
	return intersection;
}

template<typename TYPE, class HASH = std::hash<TYPE> >
std::unordered_set<TYPE, HASH> setUnion(const std::unordered_set<TYPE, HASH>& in1, const std::unordered_set<TYPE, HASH>& in2) {
	std::unordered_set<TYPE, HASH> out;
	out.reserve(in1.size() + in2.size());
	out.insert(in1.begin(), in1.end());
	out.insert(in2.begin(), in2.end());
	return out;
}

template<typename TYPE, class HASH = std::hash<TYPE> >
std::unordered_set<TYPE, HASH> setDifference(const std::unordered_set<TYPE, HASH>& in1, const std::unordered_set<TYPE, HASH>& in2) {
	std::unordered_set<TYPE, HASH> out;
	out.reserve(in1.size() + in2.size());
	for (const TYPE& element : in1) {
		if (in2.find(element) == in2.end()) {
			out.insert(element);
		}
	}
	for (const TYPE& element : in2) {
		if (in1.find(element) == in1.end()) {
			out.insert(element);
		}
	}
	return out;
}

}
