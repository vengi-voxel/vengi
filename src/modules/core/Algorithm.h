/**
 * @file
 */

namespace core {

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

}
