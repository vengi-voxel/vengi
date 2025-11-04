/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <stddef.h>

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
	for (int i = n; i > 0; --i) {
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

template<class Iter>
Iter rotate(Iter first, Iter middle, Iter last) {
	if (first == middle)
		return last;

	if (middle == last)
		return first;

	Iter write = first;
	Iter next_read = first; // read position for when "read" hits "last"

	for (Iter read = middle; read != last; ++write, ++read) {
		if (write == next_read)
			next_read = read; // track where "first" went
		core::exchange(*write, *read);
	}

	// rotate the remaining sequence into place
	rotate(write, next_read, last);
	return write;
}

template<typename Iter, typename T, class Comparator>
Iter lower_bound(Iter first, Iter last, const T& val, Comparator comp) {
	int len = core::distance(first, last);
	while (len > 0) {
		int half = len / 2;
		Iter middle = first;
		for (int i = 0; i < half; ++i) {
			++middle;
		}

		if (comp(*middle, val)) {
			first = middle;
			++first;
			len = len - half - 1;
		} else {
			len = half;
		}
	}
	return first;
}

template<typename Iter, typename T, class Comparator>
Iter upper_bound(Iter first, Iter last, const T& val, Comparator comp) {
	int len = core::distance(first, last);
	while (len > 0) {
		int half = len / 2;
		Iter middle = first;
		for (int i = 0; i < half; ++i) {
			++middle;
		}

		if (!comp(val, *middle)) {
			first = middle;
			++first;
			len = len - half - 1;
		} else {
			len = half;
		}
	}
	return first;
}

template<class Iter>
Iter rotate_forward(Iter first, Iter middle, Iter last) {
	if (first == middle)
		return last;
	if (middle == last)
		return first;

	Iter next = middle;
	while (first != next) {
		core::exchange(*first, *next);
		++first;
		++next;
		if (next == last) {
			next = middle;
		} else if (first == middle) {
			middle = next;
		}
	}
	return first;
}

namespace priv {

// In-place merge using rotation-based algorithm with binary search
template<typename Iter, class Comparator>
void inplace_merge_impl(Iter first, Iter middle, Iter last, Comparator comp) {
	if (first == middle || middle == last) {
		return;
	}

	const int len1 = core::distance(first, middle);
	const int len2 = core::distance(middle, last);

	// Handle trivial cases
	if (len1 == 0 || len2 == 0) {
		return;
	}

	// Check if already sorted
	Iter last1 = middle;
	--last1;
	if (!comp(*middle, *last1)) {
		return;
	}

	// For small ranges, use simple insertion-based merge
	if (len1 + len2 < 15) {
		while (first != middle && middle != last) {
			if (comp(*middle, *first)) {
				// Need to insert *middle before *first
				auto tmp = *middle;
				Iter current = middle;
				while (current != first) {
					Iter prev = current;
					--prev;
					*current = *prev;
					current = prev;
				}
				*first = tmp;
				++middle;
			}
			++first;
		}
		return;
	}

	// For larger ranges, use divide-and-conquer with binary search
	// Always split the first sequence in half
	Iter cut1 = first;
	for (int i = 0; i < len1 / 2; ++i) {
		++cut1;
	}

	// Find where *cut1 should go in second range using binary search
	Iter cut2 = core::lower_bound(middle, last, *cut1, comp);

	// Rotate [cut1, middle, cut2) to bring [middle, cut2) before cut1
	Iter new_middle = core::rotate(cut1, middle, cut2);

	// Recursively merge the two parts
	inplace_merge_impl(first, cut1, new_middle, comp);
	inplace_merge_impl(new_middle, cut2, last, comp);
}

}

template<typename Iter, class Comparator>
void inplace_merge(Iter first, Iter middle, Iter last, Comparator comp) {
	priv::inplace_merge_impl(first, middle, last, comp);
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

/**
 * @note This sort is unstable
 */
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
		} else if (buf1[i] == buf2[j]) {
			++i;
			++j;
		} else {
			++j;
		}
	}
	for (;i < buf1Length;) {
		out[outIdx++] = buf1[i++];
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

void *memchr_not(const void *s, int c, size_t n);

template<typename T, typename Compare>
static size_t nth_element_partition(T& arr, size_t left, size_t right, size_t pivotIndex, Compare comp) {
	typename T::value_type pivot = arr[pivotIndex];
	core::exchange(arr[pivotIndex], arr[right - 1]);

	size_t storeIndex = left;
	for (size_t i = left; i < right - 1; ++i) {
		if (comp(arr[i], pivot)) {
			core::exchange(arr[storeIndex], arr[i]);
			storeIndex++;
		}
	}

	core::exchange(arr[storeIndex], arr[right - 1]);

	return storeIndex;
}

template<typename T, typename Compare>
void nth_element(T &arr, size_t left, size_t right, size_t n, Compare comp) {
	if (left >= right)
		return;

	const size_t pivotIndex = nth_element_partition(arr, left, right, left + (right - left) / 2, comp);
	if (n == pivotIndex) {
		return;
	}
	if (n < pivotIndex) {
		nth_element(arr, left, pivotIndex, n, comp);
	} else {
		nth_element(arr, pivotIndex + 1, right, n, comp);
	}
}

}
