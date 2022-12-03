/**
 * @file
 */

#pragma once

namespace core {

template <typename FIRST, typename SECOND> struct Pair {
	FIRST first{};
	SECOND second{};

	constexpr Pair() {
	}

	constexpr Pair(FIRST &&_first, SECOND &&_second) : first(_first), second(_second) {
	}

	Pair(const FIRST &_first, const SECOND &_second) : first(_first), second(_second) {
	}
};

} // namespace core
