/**
 * @file
 */

#pragma once

#include <unordered_set>
#include <algorithm>
#include <vector>
#include <math.h>

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

template<typename KEY, typename VALUE, class HASH = std::hash<KEY> >
std::unordered_set<KEY, HASH> mapKeysDifference(const std::unordered_map<KEY, VALUE, HASH>& in1, const std::unordered_map<KEY, VALUE, HASH>& in2) {
	std::unordered_set<KEY, HASH> keys1(in1.size());
	std::unordered_set<KEY, HASH> keys2(in2.size());
	auto key_selector = [](auto pair) {return pair.first;};
	std::transform(in1.begin(), in1.end(), keys1.begin(), key_selector);
	std::transform(in2.begin(), in2.end(), keys2.begin(), key_selector);
	return setDifference(keys1, keys2);
}

template<typename KEY, typename VALUE, class HASH = std::hash<KEY> >
std::unordered_set<KEY, HASH> mapFindChangedValues(const std::unordered_map<KEY, VALUE, HASH>& in1, const std::unordered_map<KEY, VALUE, HASH>& in2) {
	std::unordered_set<KEY, HASH> result;
	for (const auto& e : in1) {
		const KEY& key = e.first;
		const auto& i = in2.find(key);
		if (i == in2.end()) {
			result.insert(key);
			continue;
		}
		const VALUE& oldValue = i->second;
		const VALUE& newValue = e.second;
		if (fabs(newValue - oldValue) > (VALUE)0.000001) {
			result.insert(key);
		}
	}
	for (const auto& e : in2) {
		const KEY& key = e.first;
		auto i = in1.find(key);
		if (i == in2.end()) {
			result.insert(key);
			continue;
		}
	}

	return result;
}

/**
 * The two input vectors must be sorted
 */
template<typename VALUE>
void vectorUnion(const std::vector<VALUE>& v1, const std::vector<VALUE>& v2, std::vector<VALUE> &out) {
	out.clear();
	std::set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter(out));
}

/**
 * The two input vectors must be sorted
 */
template<typename VALUE>
void vectorIntersection(const std::vector<VALUE>& v1, const std::vector<VALUE>& v2, std::vector<VALUE>& out) {
	out.clear();
	std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter(out));
}

}
