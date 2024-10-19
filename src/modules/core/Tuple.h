/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <stddef.h>

namespace core {

// Base case: Empty tuple
template<typename... Types>
struct Tuple {
	// Empty tuple
};

// Recursive case: Tuple with at least one element
template<typename FIRST, typename... TAIL>
struct Tuple<FIRST, TAIL...> {
	FIRST head{};
	Tuple<TAIL...> tail{};

	// Default constructor
	constexpr Tuple() {
	}

	// Variadic constructor with forwarding
	constexpr Tuple(FIRST &&_head, TAIL &&..._tail)
		: head(core::forward<FIRST>(_head)), tail(core::forward<TAIL>(_tail)...) {
	}

	// Copy constructor
	constexpr Tuple(const FIRST &_head, const TAIL &..._tail) : head(_head), tail(_tail...) {
	}
};

// Helper struct to get elements by index
template<size_t Index, typename TupleType>
struct TupleElement;

// Specialization for the head element (index 0)
template<typename FIRST, typename... TAIL>
struct TupleElement<0, Tuple<FIRST, TAIL...>> {
	using Type = FIRST;
	static constexpr FIRST &get(Tuple<FIRST, TAIL...> &tuple) {
		return tuple.head;
	}

	static constexpr const FIRST &get(const Tuple<FIRST, TAIL...> &tuple) {
		return tuple.head;
	}
};

// Recursive specialization for non-zero indices
template<size_t Index, typename FIRST, typename... TAIL>
struct TupleElement<Index, Tuple<FIRST, TAIL...>> {
	using Type = typename TupleElement<Index - 1, Tuple<TAIL...>>::Type;

	static constexpr auto &get(Tuple<FIRST, TAIL...> &tuple) {
		return TupleElement<Index - 1, Tuple<TAIL...>>::get(tuple.tail);
	}

	static constexpr const auto &get(const Tuple<FIRST, TAIL...> &tuple) {
		return TupleElement<Index - 1, Tuple<TAIL...>>::get(tuple.tail);
	}
};

// Helper function for ease of access
template<size_t Index, typename... Types>
constexpr typename TupleElement<Index, Tuple<Types...>>::Type &get(Tuple<Types...> &tuple) {
	return TupleElement<Index, Tuple<Types...>>::get(tuple);
}

template<size_t Index, typename... Types>
constexpr const typename TupleElement<Index, Tuple<Types...>>::Type &get(const Tuple<Types...> &tuple) {
	return TupleElement<Index, Tuple<Types...>>::get(tuple);
}

} // namespace core
