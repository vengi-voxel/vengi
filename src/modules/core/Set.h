#pragma once

#include <unordered_set>

namespace core {

template<typename TYPE>
inline std::unordered_set<TYPE> setIntersection(const std::unordered_set<TYPE>& set1, const std::unordered_set<TYPE>& set2) {
	if (set1.size() > set2.size())
		return setIntersection(set2, set1);

	std::unordered_set<TYPE> intersection;
	for (auto const& element : set1)
		if (set2.count(element) > 0)
			intersection.insert(element);
	return intersection;
}

template<typename TYPE>
inline std::unordered_set<TYPE> setUnion(const std::unordered_set<TYPE>& in1, const std::unordered_set<TYPE>& in2) {
	std::unordered_set<TYPE> out;
	out.reserve(in1.size() + in2.size());
	out.insert(in1.begin(), in1.end());
	out.insert(in2.begin(), in2.end());
	return out;
}

template<typename TYPE>
inline std::unordered_set<TYPE> setDifference(const std::unordered_set<TYPE>& in1, const std::unordered_set<TYPE>& in2) {
	std::unordered_set<TYPE> out;
	for (const TYPE& element : in1)
		if (in2.find(element) == in2.end())
			out.insert(element);
	for (const TYPE& element : in2)
		if (in1.find(element) == in1.end())
			out.insert(element);
	return out;
}

}
