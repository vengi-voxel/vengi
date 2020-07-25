/**
 * @file
 */

#include "core/Common.h"
namespace core {

template<class Iter>
constexpr Iter next(Iter it, int n = 1) {
	for (int i = 0; i < n; ++i) {
		++it;
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

}
