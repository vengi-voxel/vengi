/**
 * @brief
 */

#pragma once

#include <type_traits>

namespace core {

#if __cplusplus >= 201703L
template<typename F>
using invoke_result_t = typename std::invoke_result<F>::type;
#else
template<typename F>
using invoke_result_t = typename std::result_of<F()>::type;
#endif

} // namespace core
