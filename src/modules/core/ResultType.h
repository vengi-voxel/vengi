/**
 * @file
 */

#pragma once

#include "core/Common.h"

namespace core {

namespace detail {
template<typename F>
struct invoke_result {
	using type = decltype(core::declval<F>()());
};
} // namespace detail

template<typename F>
using invoke_result_t = typename detail::invoke_result<F>::type;

} // namespace core
