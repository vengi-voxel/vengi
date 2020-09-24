/**
 * @file
 */

#pragma once

#include "core/Common.h"
namespace core {

template<class Iter>
constexpr Iter next(Iter it, int n = 1) {
	for (int i = 0; i < n; ++i) {
		++it;
	}
	return it;
}

template<class Iter>
constexpr Iter prev(Iter it, int n = 1) {
	for (int i = n; i < 0; --i) {
		--it;
	}
	return it;
}

template<typename T>
constexpr int distance(T first, T last) {
	int d = 0;
	while (first != last) {
		++first;
		++d;
	}
	return d;
}

template<class Iter, class T>
Iter find(Iter first, Iter last, const T &v) {
	while (first != last) {
		if (*first == v) {
			return first;
		}
		++first;
	}
	return last;
}

template<class Iter, class Pred>
Iter find_if(Iter first, Iter last, Pred predicate) {
	while (first != last) {
		if (predicate(*first)) {
			return first;
		}
		++first;
	}
	return last;
}

template<typename Iter, class Comparator>
Iter sortRange(Iter first, Iter last, Iter split, Comparator &comp) {
	--last;
	if (split != last) {
		core::exchange(*split, *last);
	}

	Iter i = first;
	for (; first != last; ++first) {
		if (comp(*last, *first)) {
			continue;
		}
		if (first != i) {
			core::exchange(*first, *i);
		}
		++i;
	}

	if (last != i) {
		core::exchange(*last, *i);
	}
	return i;
}

template<typename Iter, class Comparator>
void sort(Iter first, Iter last, Comparator comp) {
	if (first == last) {
		return;
	}

	Iter split = first;
	const int size = core::distance(first, last);
	for (int n = size / 2; n > 0; --n) {
		++split;
	}
	split = core::sortRange<Iter, Comparator>(first, last, split, comp);
	core::sort<Iter, Comparator>(first, split, comp);
	core::sort<Iter, Comparator>(++split, last, comp);
}

/**
 * @brief Calculates the values from @c buf1 that are not part of @c buf2 and store those values in the @c out buffer.
 * @param[in] buf1 The buffer which values will be put into the @c out buffer if they are not in @c buf2
 * @param[in] buf2 The buffer which values might not be in @c buf1 in order to be added to the @c out buffer
 * @param[in] buf1Length The amount of values in the @c buf1 buffer
 * @param[in] buf2Length The amount of values in the @c buf2 buffer
 * @param[out] out The output buffer
 * @param[in] outLength The size of the output buffer
 * @param[out] outIdx The amount of values added to the output buffer
 * @note If the amount of values in the @c out buffer exceed the @c out buffer size, the loop is just aborted but the previous values
 * where added and they are valid results.
 */
template <typename Type>
void sortedDifference(const Type *buf1, int buf1Length, const Type *buf2, int buf2Length, Type *out, int outLength, int& outIdx) {
	int i = 0;
	int j = 0;
	outIdx = 0;
	while (i < buf1Length && j < buf2Length) {
		if (outLength <= outIdx) {
			return;
		}
		if (buf1[i] < buf2[j]) {
			out[outIdx++] = buf1[i++];
		} else if (buf2[j] < buf1[i]) {
			out[outIdx++] = buf2[j++];
		} else {
			++i;
			++j;
		}
	}
}

template <typename Type>
void sortedIntersection(const Type *buf1, int buf1Length, const Type *buf2, int buf2Length, Type *out, int outLength, int& outIdx) {
	int i = 0;
	int j = 0;
	outIdx = 0;
	while (i < buf1Length && j < buf2Length) {
		if (outLength <= outIdx) {
			return;
		}
		if (buf1[i] < buf2[j]) {
			++i;
		} else if (buf2[j] < buf1[i]) {
			++j;
		} else {
			out[outIdx++] = buf1[i];
			++i;
			++j;
		}
	}
}

template <typename Type>
void sortedUnion(const Type *buf1, int buf1Length, const Type *buf2, int buf2Length, Type *out, int outLength, int& outIdx) {
	int i = 0;
	int j = 0;
	outIdx = 0;
	while (i < buf1Length && j < buf2Length) {
		if (outLength <= outIdx) {
			return;
		}
		if (buf1[i] < buf2[j]) {
			out[outIdx++] = buf1[i++];
		} else if (buf1[i] == buf2[j]) {
			out[outIdx++] = buf1[i];
			++j;
			++i;
		} else if (buf1[i] > buf2[j]) {
			out[outIdx++] = buf2[j++];
		}
	}
	for (; i < buf1Length;) {
		out[outIdx++] = buf1[i++];
	}
	for (; j < buf2Length;) {
		out[outIdx++] = buf2[j++];
	}
}

}
